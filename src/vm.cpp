void reset_stack(VM* vm)
{
    vm->stack_top     = vm->stack;
    vm->frame_count   = 0;
    vm->open_upvalues = NULL;
}

static void runtime_error(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (i32 i = vm->frame_count - 1; i >= 0; i--)
    {
        CallFrame* frame = &vm->frames[i];
        ObjFunction* function = frame->closure->function;

        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);

        if (function->name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    reset_stack(vm);
}

static void define_native(VM* vm, const char* name, NativeFn function, NativeArguments arguments)
{
    push(vm, obj_val(copy_string(&vm->gc, &vm->store, &vm->strings, name, (i32)strlen(name))));
    push(vm, obj_val(new_native(&vm->gc, function, arguments, &vm->store)));
    table_set(&vm->gc, &vm->globals, as_string(vm->stack[0]), vm->stack[1]);
    pop(vm);
    pop(vm);
}

static Value clock_native(i32 arg_count, Value* args)
{
    return number_val((f64)clock() / CLOCKS_PER_SEC);
}

static Value sqrt_native(i32 arg_count, Value* args)
{
    Value value = args[0];
    return number_val(sqrt(value.as.number));
}

static Value pow_native(i32 arg_count, Value* args)
{
    Value value    = args[0];
    Value exponent = args[1];
    return number_val(pow(value.as.number, exponent.as.number));
}

static Value atof_native(i32 arg_count, Value* args)
{
    Value value = args[0];
    return number_val(atof(as_cstring(value)));
}

void init_vm(VM* vm)
{
    reset_stack(vm);
    vm->store.objects = NULL;
    vm->gc = {};
    vm->gc.vm = vm;
    vm->gc.bytes_allocated = 0;
    vm->gc.next_gc = 1024 * 1024;
    
    init_table(&vm->strings);
    init_table(&vm->globals);

    define_native(vm, "clock", clock_native, make_native_arguments(0));
    define_native(vm, "sqrt", sqrt_native, make_native_arguments(1, ValueType::VAL_NUMBER));
    define_native(vm, "pow", pow_native, make_native_arguments(2, ValueType::VAL_NUMBER, ValueType::VAL_NUMBER));
    define_native(vm, "atof", atof_native, make_native_arguments(1, ValueType::VAL_OBJ));
}

void free_vm(VM* vm)
{
    free_table(&vm->gc, &vm->strings);
    free_table(&vm->gc, &vm->globals);    
    free_objects(&vm->store, &vm->gc);
}

static void push(VM* vm, Value value)
{
    *vm->stack_top = value;
    vm->stack_top++;
}

static Value pop(VM* vm)
{
    vm->stack_top--;
    return *vm->stack_top;
}

static Value peek(VM* vm, i32 distance)
{
    return vm->stack_top[-1 - distance];
}

static b32 call(VM* vm, ObjClosure* closure, i32 arg_count)
{
    if (arg_count != closure->function->arity)
    {
        runtime_error(vm, "Expect %d arguments but got %d", closure->function->arity, arg_count);
        return false;
    }

    if (vm->frame_count == FRAMES_MAX)
    {
        runtime_error(vm, "Stack overflow.");
        return false;
    }
    
    CallFrame* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;

    frame->slots = vm->stack_top - arg_count - 1;
    return true;
}

static b32 call_value(VM* vm, Value callee, i32 arg_count)
{
    if (is_obj(callee))
    {
        switch (callee.as.obj->type)
        {
            case OBJ_CLOSURE:
            return call(vm, as_closure(callee), arg_count);
            case OBJ_NATIVE:
            {
                ObjNative* obj = (ObjNative*)callee.as.obj;
                if(arg_count > obj->arguments.arity)
                {
                    // @Incomplete: Add formatting that reports native function name
                    runtime_error(vm, "Too many arguments in native function call.");
                    return false;            
                }
                else if(arg_count < obj->arguments.arity)
                {
                    // @Incomplete: Add formatting that reports native function name
                    runtime_error(vm, "Not enough arguments for native function call.");
                    return false;
                }

                Value* arguments = vm->stack_top - arg_count;

                for(i32 i = 0; i < arg_count; i++)
                {
                    Value arg = arguments[i];
                    ValueType type = obj->arguments.types[i];
                    if(arg.type != type)
                    {
                        // @Incomplete: Add formatting that reports native function name and argument types
                        runtime_error(vm, "Type mismatch in native function call arguments.");
                        return false;
                    }
                }
            
                NativeFn native = as_native(callee);
                Value result = native(arg_count, vm->stack_top - arg_count);
                vm->stack_top -= arg_count + 1;
                push(vm, result);
                return true;
            }
            default:
            break;
        }
    }

    runtime_error(vm, "Can only call functions and classes.");
    return false;
}

static ObjUpvalue* capture_upvalue(VM* vm, Value* local)
{
    ObjUpvalue* prev_upvalue = NULL;
    ObjUpvalue* upvalue = vm->open_upvalues;
    while(upvalue != NULL && upvalue->location > local)
    {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }

    if(upvalue != NULL && upvalue->location == local)
    {
        return upvalue;
    }
    
    ObjUpvalue* created_upvalue = new_upvalue(&vm->gc, &vm->store, local);
    created_upvalue->next = upvalue;

    if(prev_upvalue == NULL)
    {
        vm->open_upvalues = created_upvalue;
    }
    else
    {
        prev_upvalue->next = created_upvalue;
    }
    
    return created_upvalue;
}

static void close_upvalues(VM* vm, Value* last)
{
    while(vm->open_upvalues != NULL && vm->open_upvalues->location >= last)
    {
        ObjUpvalue* upvalue = vm->open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->open_upvalues = upvalue->next;
    }
}

static b32 is_falsey(Value value)
{
    return is_nil(value) || (is_bool(value) && !value.as.boolean);
}

static void concatenate(VM* vm)
{
    ObjString* b = as_string(peek(vm, 0));
    ObjString* a = as_string(peek(vm, 1));

    i32 length = a->length + b->length;
    char* chars = ALLOCATE(&vm->gc, char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = take_string(&vm->gc, &vm->store, &vm->strings, chars, length);
    pop(vm);
    pop(vm);
    push(vm, obj_val(result));
}

InterpretResult interpret(VM* vm, const char* source)
{
    ObjFunction* function = compile(&vm->gc, source, &vm->store, &vm->strings);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(vm, obj_val(function));
    ObjClosure* closure = new_closure(&vm->gc, function, &vm->store);
    pop(vm);
    push(vm, obj_val(closure));
    call(vm, closure, 0);
    
    return run(vm);
}

static InterpretResult run(VM* vm)
{
    CallFrame* frame = &vm->frames[vm->frame_count - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (u16)(frame->ip[-2] << 8) | frame->ip[-1])
#define READ_STRING() (as_string(READ_CONSTANT()))

#define BINARY_OP(value_type, op)                               \
    do {                                                        \
        if (!is_number(peek(vm, 0)) || !is_number(peek(vm, 1))) \
        {                                                       \
            runtime_error(vm, "Operands must be numbers.");     \
            return INTERPRET_RUNTIME_ERROR;                     \
        }                                                       \
        f64 b = pop(vm).as.number;                              \
        f64 a = pop(vm).as.number;                              \
        push(vm, value_type(a op b));                           \
    } while(false)

    for(;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for(Value* slot = vm->stack; slot < vm->stack_top; slot++)
        {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(&frame->closure->function->chunk,
                                (i32)(frame->ip - frame->closure->function->chunk.code));
#endif
        u8 instruction;
        switch(instruction = READ_BYTE())
        {
            case OP_PRINT:
            {
                print_value(pop(vm));
                printf("\n");
            }
            break;
            case OP_JUMP:
            {
                u16 offset = READ_SHORT();
                frame->ip += offset;
            }
            break;
            case OP_JUMP_IF_FALSE:
            {
                u16 offset = READ_SHORT();
                if (is_falsey(peek(vm, 0))) frame->ip += offset;
            }
            break;
            case OP_COMPARE:
            {
                Value b = peek(vm, 0);
                Value a = peek(vm, 1);
                push(vm, bool_val(values_equal(a, b)));
            }
            break;
            case OP_LOOP:
            {
                u16 offset = READ_SHORT();
                frame->ip -= offset;
            }
            break;
            case OP_CALL:
            {
                i32 arg_count = READ_BYTE();
                if (!call_value(vm, peek(vm, arg_count), arg_count))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frame_count - 1];
            }
            break;
            case OP_CLOSURE:
            {
                ObjFunction* function = as_function(READ_CONSTANT());
                ObjClosure* closure = new_closure(&vm->gc, function, &vm->store);
                push(vm, obj_val(closure));
                for(i32 i = 0; i < closure->upvalue_count; i++)
                {
                    u8 is_local = READ_BYTE();
                    u8 index = READ_BYTE();
                    if(is_local)
                    {
                        closure->upvalues[i] = capture_upvalue(vm, frame->slots + index);
                    }
                    else
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
            }
            break;
            case OP_CLASS:
            {
                push(vm, obj_val(new_class(&vm->gc, &vm->store, READ_STRING())));
            }
            break;
            case OP_CLOSE_UPVALUE:
            {
                close_upvalues(vm, vm->stack_top - 1);
                pop(vm);
            }
            break;
            case OP_RETURN:
            {
                Value result = pop(vm);
                close_upvalues(vm, frame->slots);
                vm->frame_count--;
                if (vm->frame_count == 0)
                {
                    pop(vm);
                    // Exit interpreter
                    return INTERPRET_OK;
                }

                vm->stack_top = frame->slots;
                push(vm, result);

                frame = &vm->frames[vm->frame_count - 1];
            }
            break;
            case OP_CONSTANT:
            {
                Value constant = READ_CONSTANT();
                push(vm, constant);
            }
            break;
            case OP_NIL: push(vm, nil_val()); break;
            case OP_TRUE: push(vm, bool_val(true)); break;
            case OP_FALSE: push(vm, bool_val(false)); break;
            case OP_POP: pop(vm); break;
            case OP_GET_LOCAL:
            {
                u8 slot = READ_BYTE();
                push(vm, frame->slots[slot]);
            }
            break;
            case OP_GET_GLOBAL:
            {
                ObjString* name = READ_STRING();
                Value value;
                if (!table_get(&vm->globals, name, &value))
                {
                    runtime_error(vm, "Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, value);
            }
            break;
            case OP_DEFINE_GLOBAL:
            {
                ObjString* name = READ_STRING();
                table_set(&vm->gc, &vm->globals, name, peek(vm, 0));
                pop(vm);
            }
            break;
            case OP_SET_LOCAL:
            {
                u8 slot = READ_BYTE();
                frame->slots[slot] = peek(vm, 0);
            }
            break;
            case OP_SET_GLOBAL:
            {
                ObjString* name = READ_STRING();
                if (table_set(&vm->gc, &vm->globals, name, peek(vm, 0)))
                {
                    table_delete(&vm->globals, name);
                    runtime_error(vm, "Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
            }
            break;
            case OP_GET_UPVALUE:
            {
                u8 slot = READ_BYTE();
                push(vm, *frame->closure->upvalues[slot]->location);
            }
            break;
            case OP_SET_UPVALUE:
            {
                u8 slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(vm, 0);
            }
            break;
            case OP_EQUAL:
            {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, bool_val(values_equal(a, b)));
            }
            break;
            case OP_GREATER:  BINARY_OP(bool_val, >);   break;
            case OP_LESS:     BINARY_OP(bool_val, <);   break;
            case OP_ADD:
            {
                if (is_string(peek(vm, 0)) && is_string(peek(vm, 1)))
                {
                    concatenate(vm);
                }
                else if (is_number(peek(vm, 0)) && is_number(peek(vm, 1)))
                {
                    f64 b = pop(vm).as.number;
                    f64 a = pop(vm).as.number;
                    push(vm, number_val(a + b));
                }
                else
                {
                    runtime_error(vm, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            }
            break;
            case OP_SUBTRACT: BINARY_OP(number_val, -); break;
            case OP_MULTIPLY: BINARY_OP(number_val, *); break;
            case OP_DIVIDE:   BINARY_OP(number_val, /); break;
            case OP_NOT:
            {
                push(vm, bool_val(is_falsey(pop(vm))));
            }
            break;
            case OP_NEGATE:
            {
                if (!is_number(peek(vm, 0)))
                {
                    runtime_error(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
            
                push(vm, number_val(-pop(vm).as.number));
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef READ_STRING
#undef BINARY_OP
}

void free_objects(ObjectStore* store, GarbageCollector* gc)
{
    Obj* object = store->objects;
    while (object != NULL)
    {
        Obj* next = object->next;
        free_object(gc, object);
        object = next;
    }
    store->objects = NULL;

    free(gc->gray_stack);
}
