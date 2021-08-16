#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================e
struct Obj;
struct ObjString;

enum ValueType
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
};

#ifdef NAN_BOXING

typedef u64 Value;

#define NIL_VAL   ((Value)(u64)(QNAN | TAG_NIL))
#define TRUE_VAL  ((Value)(u64)(QNAN | TAG_TRUE))
#define FALSE_VAL ((Value)(u64)(QNAN | TAG_FALSE))

#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)

#define AS_OBJ(value)    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define AS_BOOL(value)   ((value == TRUE_VAL))
#define AS_NUMBER(value) (value_to_num(value))

static inline Value num_to_value(f64 num)
{
    Value value;
    memcpy(&value, &num, sizeof(f64));
    return value;
}

Value number_val(f64 number)
{
    return num_to_value(number);
}

Value nil_val()
{
    return NIL_VAL;
}

Value bool_val(b32 val)
{
    return BOOL_VAL(val);
}

Value obj_val(Obj* obj)
{
    return (Value)(SIGN_BIT | QNAN | (u64)(uintptr_t)(obj));    
}
#define OBJ_VAL(object) (obj_val((Obj*)object))

static inline f64 value_to_num(Value value)
{
    f64 num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

#define IS_OBJ(value)    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define IS_BOOL(value)   ((value | 1) == TRUE_VAL)
#define IS_NIL(value)    ((value) == NIL_VAL)
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)

#else

#define IS_NUMBER(value) (value.type == VAL_NUMBER)
#define IS_BOOL(value) (value.type == VAL_BOOL)
#define IS_NIL(value) (value.type == VAL_NIL)
#define IS_OBJ(value) (value.type == VAL_OBJ)

#define AS_NUMBER(value) (value.as.number)
#define AS_BOOL(value) (value.as.boolean)
#define AS_OBJ(value) (value.as.obj)

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

Value number_val(f64 number)
{
    Value value = {};
    value.type = VAL_NUMBER;
    value.as.number = number;
    return value;
}

Value bool_val(b32 boolean)
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

Value obj_val(Obj* object)
{
    Value value = {};
    value.type = VAL_OBJ;
    value.as.obj = object;
    return value;
}

#define OBJ_VAL(object) (obj_val((Obj*)object))

#endif

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
// =================================================================

#endif
