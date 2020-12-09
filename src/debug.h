#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

i32 disassemble_instruction(Chunk* chunk, i32 offset);
void disassemble_chunk(Chunk* chunk, const char* name);
static i32 simple_instruction(const char* name, i32 offset);
static i32 constant_instruction(const char* name, Chunk* chunk, i32 offset);
static i32 constant_long_instruction(const char* name, Chunk* chunk, i32 offset);

void disassemble_chunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);

    for(i32 offset = 0; offset < chunk->count;)
    {
        offset = disassemble_instruction(chunk, offset);
    }
}

i32 disassemble_instruction(Chunk* chunk, i32 offset)
{
    printf("%04d ", offset);
    if(offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", chunk->lines[offset]);
    }

    u8 instruction = chunk->code[offset];
    switch(instruction)
    {
    case OP_CONSTANT:
    {
        return constant_instruction("OP_CONSTANT", chunk, offset);
    }
    case OP_CONSTANT_LONG:
    {
        return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
    }
    case OP_ADD:
    {
        return simple_instruction("OP_ADD", offset);
    }
    case OP_SUBTRACT:
    {
        return simple_instruction("OP_SUBTRACT", offset);
    }
    case OP_MULTIPLY:
    {
        return simple_instruction("OP_MULTIPLY", offset);
    }
    case OP_DIVIDE:
    {
        return simple_instruction("OP_DIVIDE", offset);
    }
    case OP_NEGATE:
    {
        return simple_instruction("OP_NEGATE", offset);
    }
    case OP_RETURN:
    {
        return simple_instruction("OP_RETURN", offset);
    }
    default:
    {
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
    }
}

static i32 simple_instruction(const char* name, i32 offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static i32 constant_instruction(const char* name, Chunk* chunk, i32 offset)
{
    u8 constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static i32 constant_long_instruction(const char* name, Chunk* chunk, i32 offset)
{
    i32 constant = (chunk->code[offset + 1])
        | (chunk->code[offset + 2] << 8)
        | (chunk->code[offset + 3] << 16);
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

#endif
