#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================
using Value = double;
struct ValueArray
{
    i32 capacity;
    i32 count;
    Value* values;
};
// =================================================================

// =================================================================
// API Functions
// =================================================================
void init_value_array(ValueArray* array);
void free_value_array(ValueArray* array);
void write_value_array(ValueArray* array, Value value);
void print_value(Value value);
// =================================================================

#ifdef CLOX_VALUE_IMPLEMENTATION

void init_value_array(ValueArray* array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void free_value_array(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

void write_value_array(ValueArray* array, Value value)
{
    if(array->capacity < array->count + 1)
    {
        i32 old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void print_value(Value value)
{
    printf("%g", value);
}

#endif

#endif
