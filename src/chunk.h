#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

enum OpCode
{
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
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

void init_chunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_value_array(&chunk->constants);
}

void free_chunk(Chunk* chunk)
{
    FREE_ARRAY(u8, chunk->code, chunk->capacity);
    FREE_ARRAY(i32, chunk->lines, chunk->capacity);
    free_value_array(&chunk->constants);
    init_chunk(chunk);
}

void write_chunk(Chunk* chunk, u8 byte, i32 line)
{
    if(chunk->capacity < chunk->count + 1)
    {
        i32 old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(u8, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(i32, chunk->lines, old_capacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

i32 add_constant(Chunk* chunk, Value value)
{
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void write_constant(Chunk* chunk, Value value, i32 line)
{
    i32 constant = add_constant(chunk, value);
    if(constant >= 1 << 8)
    {
        write_chunk(chunk, OP_CONSTANT_LONG, line);
        write_chunk(chunk, (u8)(constant & 0x000000ff), line);
        write_chunk(chunk, (u8)((constant & 0x0000ff00) >> 8), line);
        write_chunk(chunk, (u8)((constant & 0x00ff0000) >> 16), line);
    }
    else
    {
        write_chunk(chunk, OP_CONSTANT, line);
        write_chunk(chunk, constant, line);
    }
}

#endif
