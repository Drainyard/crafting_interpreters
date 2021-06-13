void init_chunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_value_array(&chunk->constants);
}

void free_chunk(GarbageCollector* gc, Chunk* chunk)
{
    FREE_ARRAY(gc, u8, chunk->code, chunk->capacity);
    FREE_ARRAY(gc, i32, chunk->lines, chunk->capacity);
    free_value_array(gc, &chunk->constants);
    init_chunk(chunk);
}

void write_chunk(GarbageCollector* gc, Chunk* chunk, u8 byte, i32 line)
{
    if(chunk->capacity < chunk->count + 1)
    {
        i32 old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(gc, u8, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(gc, i32, chunk->lines, old_capacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

i32 add_constant(GarbageCollector* gc, Chunk* chunk, Value value)
{
    push(gc->vm, value);
    write_value_array(gc, &chunk->constants, value);
    pop(gc->vm);
    return chunk->constants.count - 1;
}

void write_constant(GarbageCollector* gc, Chunk* chunk, Value value, i32 line)
{
    i32 constant = add_constant(gc, chunk, value);
    if(constant >= 1 << 8)
    {
        write_chunk(gc, chunk, OP_CONSTANT_LONG, line);
        write_chunk(gc, chunk, (u8)(constant & 0x000000ff), line);
        write_chunk(gc, chunk, (u8)((constant & 0x0000ff00) >> 8), line);
        write_chunk(gc, chunk, (u8)((constant & 0x00ff0000) >> 16), line);
    }
    else
    {
        write_chunk(gc, chunk, OP_CONSTANT, line);
        write_chunk(gc, chunk, (u8)constant, line);
    }
}
