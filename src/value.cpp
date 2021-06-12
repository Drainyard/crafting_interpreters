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

Value obj_val(ObjClosure* object)
{
    return obj_val((Obj*)object);
}

Value obj_val(ObjFunction* object)
{
    return obj_val((Obj*)object);
}

Value obj_val(ObjNative* object)
{
    return obj_val((Obj*)object);
}

Value obj_val(ObjString* object)
{
    return obj_val((Obj*)object);
}

b32 is_number(Value value)
{
    return value.type == VAL_NUMBER;
}

b32 is_bool(Value value)
{
    return value.type == VAL_BOOL;
}

b32 is_nil(Value value)
{
    return value.type == VAL_NIL;
}

b32 is_obj(Value value)
{
    return value.type == VAL_OBJ;
}

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
    case VAL_OBJ:
    print_object(value); break;
    }    
}

b32 values_equal(Value a, Value b)
{
    if (a.type != b.type) return false;

    switch(a.type)
    {
    case VAL_BOOL:   return a.as.boolean == b.as.boolean;
    case VAL_NIL:    return true;
    case VAL_NUMBER: return a.as.number == b.as.number;
    case VAL_OBJ: return a.as.obj == b.as.obj;
    default:
    return false;
    }
}
