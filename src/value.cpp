void init_value_array(ValueArray* array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void free_value_array(GarbageCollector* gc, ValueArray* array)
{
    FREE_ARRAY(gc, Value, array->values, array->capacity);
    init_value_array(array);
}

void write_value_array(GarbageCollector* gc, ValueArray* array, Value value)
{
    if(array->capacity < array->count + 1)
    {
        i32 old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(gc, Value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void print_value(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value))
    {
        printf(AS_BOOL(value) ? "true" : "false");
    }
    else if (IS_NIL(value))
    {
        printf("nil");
    }
    else if (IS_NUMBER(value))
    {
        printf("%g", AS_NUMBER(value));
    }
    else if (IS_OBJ(value))
    {
        print_object(value);
    }
#else
    switch(value.type)
    {
        case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false"); break;    
        case VAL_NIL:
        printf("nil"); break;
        case VAL_NUMBER:
        printf("%g", AS_NUMBER(value)); break;
        case VAL_OBJ:
        print_object(value); break;
    }    
#endif
}

b32 values_equal(Value a, Value b)
{
#ifdef NAN_BOXING
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    
    return a == b;
#else
    if (a.type != b.type) return false;

    switch(a.type)
    {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:    return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
        default:
        return false;
    }
#endif
}
