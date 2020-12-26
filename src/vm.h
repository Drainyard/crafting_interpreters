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

    Table strings;
    Table globals;

    ObjectStore store;
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
static void concatenate(VM* vm);
static void runtime_error(VM* vm, const char* format, ...);
void free_objects(VM* vm);
// =================================================================

#endif
