void reset_stack(VM* vm)
{
    vm->stack_top = vm->stack;
}

static void runtime_error(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->ip - vm->chunk->code - 1;
    i32 line = vm->chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    reset_stack(vm);
}

void init_vm(VM* vm)
{
    reset_stack(vm);
    vm->store.objects = NULL;
    init_table(&vm->strings);
    init_table(&vm->globals);
}

void free_vm(VM* vm)
{
    free_table(&vm->strings);
    free_table(&vm->globals);    
    free_objects(vm);
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

static bool is_falsey(Value value)
{
    return is_nil(value) || (is_bool(value) && !value.as.boolean);
}

static void concatenate(VM* vm)
{
    ObjString* b = as_string(pop(vm));
    ObjString* a = as_string(pop(vm));

    i32 length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = take_string(&vm->store, &vm->strings, chars, length);
    push(vm, obj_val(result));
}

InterpretResult interpret(VM* vm, const char* source)
{
    Chunk chunk = {};
    init_chunk(&chunk);

    if (!compile(source, &chunk, &vm->store, &vm->strings))
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);
    free_chunk(&chunk);
    // free_objects(vm);
    return result;
}

static InterpretResult run(VM* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_STRING() (as_string(READ_CONSTANT()))

#define BINARY_OP(value_type, op)                               \
    do {                                                        \
        if (!is_number(peek(vm, 0)) || !is_number(peek(vm, 1))) \
        {                                                       \
            runtime_error(vm, "Operands must be numbers.");     \
            return INTERPRET_RUNTIME_ERROR;                     \
        }                                                       \
        f64 b = pop(vm).as.number;                                 \
        f64 a = pop(vm).as.number;                                 \
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
        disassemble_instruction(vm->chunk, (i32)(vm->ip - vm->chunk->code));
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
        case OP_RETURN:
        {
            // Exit interpreter
            return INTERPRET_OK;
        }
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
            push(vm, vm->stack[slot]);
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
            table_set(&vm->globals, name, peek(vm, 0));
            pop(vm);
        }
        break;
        case OP_SET_LOCAL:
        {
            u8 slot = READ_BYTE();
            vm->stack[slot] = peek(vm, 0);
        }
        break;
        case OP_SET_GLOBAL:
        {
            ObjString* name = READ_STRING();
            if (table_set(&vm->globals, name, peek(vm, 0)))
            {
                table_delete(&vm->globals, name);
                runtime_error(vm, "Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
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
#undef READ_STRING
#undef BINARY_OP
}

void free_objects(VM* vm)
{
    Obj* object = vm->store.objects;
    while (object != NULL)
    {
        Obj* next = object->next;
        free_object(object);
        object = next;
    }
    vm->store.objects = NULL;
}
