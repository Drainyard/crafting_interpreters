#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

// =================================================================
// API
// =================================================================

// =================================================================
// API Functions
// =================================================================
void* reallocate(void* pointer, size_t old_size, size_t new_size);
// =================================================================

#ifdef CLOX_MEMORY_IMPLEMENTATION

// =================================================================
// Internal macros
// =================================================================
#define GROW_CAPACITY(capacity)                             \
                      ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type*)reallocate(pointer, sizeof(type) * (old_count), \
                      sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count) \
    reallocate(pointer, sizeof(type) + (old_count), 0)
// =================================================================

void* reallocate(void* pointer, size_t old_size, size_t new_size)
{
    if(new_size == 0)
    {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if(result == NULL) exit(1);
    return result;
}

#else
#define GROW_CAPACITY(capacity)
#define GROW_ARRAY(type, pointer, old_count, new_count)
#define FREE_ARRAY(type, pointer, old_count)
#endif

#endif
