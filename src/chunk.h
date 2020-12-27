#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================
enum OpCode
{
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NIL,
    OP_FALSE,
    OP_TRUE,
    OP_POP,
    OP_GET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_LOCAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN
};

struct Chunk
{
    u8* code;
    i32 count;
    i32 capacity;
    i32* lines;
    ValueArray constants;
};
// =================================================================

// =================================================================
// API Functions
// =================================================================
void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, u8 byte, i32 line);
i32 add_constant(Chunk* chunk, Value value);
void write_constant(Chunk* chunk, Value value, i32 line);
// =================================================================

#endif
