#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

// =================================================================
// API
// =================================================================

// =================================================================
// API Functions
// =================================================================
i32 disassemble_instruction(Chunk* chunk, i32 offset);
void disassemble_chunk(Chunk* chunk, const char* name);
// =================================================================

// =================================================================
// Internal Functions
// =================================================================
static i32 simple_instruction(const char* name, i32 offset);
static i32 byte_instruction(const char* name, Chunk* chunk, i32 offset);
static i32 constant_instruction(const char* name, Chunk* chunk, i32 offset);
static i32 constant_long_instruction(const char* name, Chunk* chunk, i32 offset);
// =================================================================

#endif
