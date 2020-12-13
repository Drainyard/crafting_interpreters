#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================
enum ValueType
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER
};

struct Value
{
    ValueType type;
    union
    {
        bool boolean;
        f64 number;
    } as;    
};

Value number_val(f64 number)
{
    Value value = {};
    value.type = VAL_NUMBER;
    value.as.number = number;
    return value;
}

Value bool_val(bool boolean)
{
    Value value = {};
    value.type = VAL_BOOL;
    value.as.boolean = boolean;
    return value;
}

Value nil_val()
{
    Value value = {};
    value.type = VAL_NIL;
    return value;    
}

bool is_number(Value value)
{
    return value.type == VAL_NUMBER;
}

bool is_bool(Value value)
{
    return value.type == VAL_BOOL;
}

bool is_nil(Value value)
{
    return value.type == VAL_NIL;
}

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
bool values_equal(Value a, Value b);
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
    switch(value.type)
    {
    case VAL_BOOL:
        printf(value.as.boolean ? "true" : "false"); break;    
    case VAL_NIL:
        printf("nil"); break;
    case VAL_NUMBER:
        printf("%g", value.as.number); break;
    }    
}

bool values_equal(Value a, Value b)
{
    if (a.type != b.type) return false;

    switch(a.type)
    {
    case VAL_BOOL:   return a.as.boolean == b.as.boolean;
    case VAL_NIL:    return true;
    case VAL_NUMBER: return a.as.number == b.as.number;
    default:
    return false;
    }
}

#endif

#endif
