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
    VAL_NUMBER,
    VAL_OBJ
};

struct Obj;
struct ObjString;

struct Value
{
    ValueType type;
    union
    {
        bool boolean;
        f64 number;
        Obj* obj;
    } as;    
};

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
Value number_val(f64 number);
Value bool_val(bool boolean);
Value nil_val();
Value obj_val(Obj* object);
Value obj_val(ObjString* object);
bool is_number(Value value);
bool is_bool(Value value);
bool is_nil(Value value);
bool is_obj(Value value);
// =================================================================

#endif
