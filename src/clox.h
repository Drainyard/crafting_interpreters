#ifndef CLOX_H
#define CLOX_H

#include "common.h"
#include "memory.h"
#include "value.h"
#include "table.h"
#include "chunk.h"
#include "object.h"
#include "debug.h"
#include "scanner.h"
#include "compiler.h"
#include "vm.h"

void repl(VM* vm);
char* read_file(const char* path);
void run_file(VM* vm, const char* path);

#ifdef CLOX_IMPLEMENTATION
#include "memory.cpp"
#include "value.cpp"
#include "object.cpp"
#include "table.cpp"
#include "chunk.cpp"
#include "debug.cpp"
#include "scanner.cpp"
#include "compiler.cpp"
#include "vm.cpp"

void repl(VM* vm)
{
    char line[1024];
    for(;;)
    {
        printf("> ");

        if(!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }
        interpret(vm, line);
    }
}

char* read_file(const char* path)
{
    FILE* file = fopen(path, "rb");

    if(!file)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);
    if(!buffer)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if(bytes_read < file_size)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

void run_file(VM* vm, const char* path)
{
    char* source = read_file(path);
    InterpretResult result = interpret(vm, source);
    free(source);

    if(result == INTERPRET_COMPILE_ERROR) exit(65);
    if(result == INTERPRET_RUNTIME_ERROR) exit(70);
}
#endif

#endif
