#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================e
enum ValueType
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
};

#define IS_NUMBER(value) (value.type == VAL_NUMBER)
#define IS_BOOL(value) (value.type == VAL_BOOL)
#define IS_NIL(value) (value.type == VAL_NIL)
#define IS_OBJ(value) (value.type == VAL_OBJ)

struct Obj;
struct ObjString;

struct Value
{
    ValueType type;
    union
    {
        b32 boolean;
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
void free_value_array(struct GarbageCollector* gc, ValueArray* array);
void write_value_array(struct GarbageCollector* gc, ValueArray* array, Value value);
void print_value(Value value);
b32 values_equal(Value a, Value b);
Value number_val(f64 number);
Value bool_val(b32 boolean);
Value nil_val();
Value obj_val(Obj* object);
// =================================================================

#endif
