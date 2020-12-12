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
void push(VM* vm, Value value);
Value pop(VM* vm);
InterpretResult interpret(VM* vm, const char* source);

// =================================================================

// =================================================================
// Internal Functions
// =================================================================
static InterpretResult run(VM* vm);
// =================================================================

#ifdef CLOX_VM_IMPLEMENTATION

void reset_stack(VM* vm)
{
    vm->stack_top = vm->stack;
}

void init_vm(VM* vm)
{
    reset_stack(vm);
}

void free_vm(VM* vm)
{
    
}

void push(VM* vm, Value value)
{
    *vm->stack_top = value;
    vm->stack_top++;
}

Value pop(VM* vm)
{
    vm->stack_top--;
    return *vm->stack_top;
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

#define BINARY_OP(op) \
    do { \
        f64 b = pop(vm); \
        f64 a = pop(vm); \
        push(vm, a op b); \
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
        case OP_ADD: BINARY_OP(+); break;
        case OP_SUBTRACT: BINARY_OP(-); break;
        case OP_MULTIPLY: BINARY_OP(*); break;
        case OP_DIVIDE: BINARY_OP(/); break;
        case OP_NEGATE:
        {
            push(vm, -pop(vm));
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
