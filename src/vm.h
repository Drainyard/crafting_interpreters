#ifndef CLOX_VM_H
#define CLOX_VM_H

// =================================================================
// API
// =================================================================

#define STACK_MAX 256

// =================================================================
// Types
// =================================================================
struct VM
{
    Chunk* chunk;
    u8* ip;
    Value stack[STACK_MAX];
    Value* stack_top;
};

enum InterpretResult
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
};
// =================================================================

// =================================================================
// API Functions
// =================================================================
void reset_stack(VM* vm);
void init_vm(VM* vm);
void free_vm(VM* vm);
InterpretResult interpret(VM* vm, const char* source);

// =================================================================

// =================================================================
// Internal Functions
// =================================================================
static InterpretResult run(VM* vm);
static void push(VM* vm, Value value);
static Value pop(VM* vm);
static Value peek(VM* vm, i32 distance);
static bool is_falsey(Value value);
static void runtime_error(VM* vm, const char* format, ...);
// =================================================================

#ifdef CLOX_VM_IMPLEMENTATION

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
}

void free_vm(VM* vm)
{
    
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

InterpretResult interpret(VM* vm, const char* source)
{
    Chunk chunk = {};
    init_chunk(&chunk);

    if (!compile(source, &chunk))
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    InterpretResult result = run(vm);
    free_chunk(&chunk);
    return result;
}

static InterpretResult run(VM* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

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
        case OP_RETURN:
        {
            print_value(pop(vm));
            printf("\n");
            return INTERPRET_OK;
        }
        case OP_CONSTANT:
        {
            Value constant = READ_CONSTANT();
            push(vm, constant);
            break;
        }
        case OP_NIL: push(vm, nil_val()); break;
        case OP_TRUE: push(vm, bool_val(true)); break;
        case OP_FALSE: push(vm, bool_val(false)); break;
        case OP_EQUAL:
        {
            Value b = pop(vm);
            Value a = pop(vm);
            push(vm, bool_val(values_equal(a, b)));
        }
        break;
        case OP_GREATER:  BINARY_OP(bool_val, >);   break;
        case OP_LESS:     BINARY_OP(bool_val, <);   break;
        case OP_ADD:      BINARY_OP(number_val, +); break;
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
#undef BINARY_OP
}

#endif
#endif
