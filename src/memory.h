#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

// =================================================================
// API
// =================================================================

struct GarbageCollector
{
    struct VM* vm;
    
    i32 gray_count;
    i32 gray_capacity;
    struct Obj** gray_stack;

    size_t bytes_allocated;
    size_t next_gc;
};

// =================================================================
// API Functions
// =================================================================
void* reallocate(GarbageCollector* gc, void* pointer, size_t old_size, size_t new_size);
void mark_value(GarbageCollector* gc, Value value);
void mark_object(GarbageCollector* gc, Obj* object);
void collect_garbage(GarbageCollector* gc);
// =================================================================

// =================================================================
// Macros
// =================================================================
#define GROW_CAPACITY(capacity)                             \
                      ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(gc, type, pointer, old_count, new_count)       \
    (type*)reallocate(gc, pointer, sizeof(type) * (old_count),   \
                      sizeof(type) * (new_count))

#define FREE_ARRAY(gc, type, pointer, old_count)             \
    reallocate(gc, pointer, sizeof(type) + (old_count), 0)

#define ALLOCATE(gc, type, count)                            \
    (type*)reallocate(gc, NULL, 0, sizeof(type) * (count))

#define FREE(gc, type, pointer) reallocate(gc, pointer, sizeof(type), 0)
// =================================================================

#endif
