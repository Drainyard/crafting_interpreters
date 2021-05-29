// @Incomplete: Consider all these functions as macros for inlining and efficiency?
static inline bool is_obj_type(Value value, ObjType type)
{
    return is_obj(value) && value.as.obj->type == type;
}

bool is_string(Value value)
{
    return is_obj_type(value, OBJ_STRING);
}

ObjString* as_string(Value value)
{
    return (ObjString*)value.as.obj;
}

char* as_cstring(Value value)
{
    return ((ObjString*)value.as.obj)->chars;
}

ObjFunction* as_function(Value value)
{
    return (ObjFunction*)value.as.obj;
}

NativeFn as_native(Value value)
{
    return ((ObjNative*)value.as.obj)->function;
}

static Obj* allocate_object(ObjectStore* store, size_t size, ObjType type)
{
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = store->objects;
    store->objects = object;
    return object;
}

static ObjFunction* new_function(ObjectStore* store)
{
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    init_chunk(&function->chunk);
    return function;
}

static NativeArguments make_native_arguments(i32 arity, ...)
{
    NativeArguments arguments = {};
    arguments.arity = arity;
    va_list args;
    va_start(args, arity);
    for(i32 i = 0; i < arity; i++)
    {
        ValueType type = va_arg(args, ValueType);
        arguments.types[i] = type;
    }
    va_end(args);
    return arguments;
}

static ObjNative* new_native(NativeFn function, NativeArguments arguments, ObjectStore* store)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function       = function;
    native->arguments      = arguments;
    return native;
}

static u32 hash_string(const char* key, i32 length)
{
    u32 hash = 2166136261u;

    for (i32 i = 0; i < length; i++)
    {
        hash ^= key[i];
        hash += 16777619;
    }

    return hash;
}

static ObjString* allocate_string(ObjectStore* store, Table* strings, const char* chars, i32 length)
{
    ObjString* string = (ObjString*)allocate_object(store, sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    string->hash = hash_string(chars, length);

    table_set(strings, string, nil_val());

    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';

    return string;
}

ObjString* copy_string(ObjectStore* store, Table* strings, const char* chars, i32 length)
{
    u32 hash = hash_string(chars, length);
    ObjString* interned = table_find_string(strings, chars, length, hash);

    if (interned) return interned;

    return allocate_string(store, strings, chars, length);
}

static void print_function(ObjFunction* function)
{
    if (function->name == NULL)
    {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

ObjString* take_string(ObjectStore* store, Table* strings, char* chars, i32 length)
{
    u32 hash = hash_string(chars, length);
    ObjString* interned = table_find_string(strings, chars, length, hash);

    if (interned)
    {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    
    return allocate_string(store, strings, chars, length);
}

void free_object(Obj* object)
{
    switch (object->type)
    {
    case OBJ_FUNCTION:
    {
        ObjFunction* function = (ObjFunction*)object;
        free_chunk(&function->chunk);
        FREE(ObjFunction, object);
    }
    case OBJ_NATIVE:
    {
        FREE(ObjNative, object);
    }
    break;
    case OBJ_STRING:
    {
        FREE(ObjString, object);
    }
    break;
    break;
    }
}

void print_object(Value value)
{
    switch(value.as.obj->type)
    {
    case OBJ_FUNCTION:
    {
        print_function((ObjFunction*)value.as.obj);
    }
    break;
    case OBJ_NATIVE:
    {
        printf("<native fn>");
    }
    break;
    case OBJ_STRING:
    {
        printf("%s", as_cstring(value));
    }
    break;    
    }
}
