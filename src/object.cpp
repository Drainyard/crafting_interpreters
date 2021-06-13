// @Incomplete: Consider all these functions as macros for inlining and efficiency?
static inline b32 is_obj_type(Value value, ObjType type)
{
    return is_obj(value) && value.as.obj->type == type;
}

b32 is_string(Value value)
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

ObjClosure* as_closure(Value value)
{
    return (ObjClosure*)value.as.obj;
}

NativeFn as_native(Value value)
{
    return ((ObjNative*)value.as.obj)->function;
}

static Obj* allocate_object(GarbageCollector* gc, ObjectStore* store, size_t size, ObjType type)
{
    Obj* object = (Obj*)reallocate(gc, NULL, 0, size);
    object->type = type;
    object->is_marked = false;
    object->next = store->objects;
    store->objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif
    
    return object;
}

ObjFunction* new_function(GarbageCollector* gc, ObjectStore* store)
{
    ObjFunction* function = ALLOCATE_OBJ(gc, ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalue_count = 0;
    function->name = NULL;
    init_chunk(&function->chunk);
    return function;
}

ObjClosure* new_closure(GarbageCollector* gc, ObjFunction* function, ObjectStore* store)
{
    ObjUpvalue** upvalues = ALLOCATE(gc, ObjUpvalue*, function->upvalue_count);
    for(i32 i = 0; i < function->upvalue_count; i++)
    {
        upvalues[i] = NULL;
    }
    ObjClosure* closure = ALLOCATE_OBJ(gc, ObjClosure, OBJ_CLOSURE);
    closure->function      = function;
    closure->upvalues      = upvalues;
    closure->upvalue_count = function->upvalue_count;
    return closure;
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

ObjNative* new_native(GarbageCollector* gc, NativeFn function, NativeArguments arguments, ObjectStore* store)
{
    ObjNative* native = ALLOCATE_OBJ(gc, ObjNative, OBJ_NATIVE);
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

static ObjString* allocate_string(GarbageCollector* gc, ObjectStore* store, Table* strings, const char* chars, i32 length)
{
    ObjString* string = (ObjString*)allocate_object(gc, store, sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    string->hash = hash_string(chars, length);

    push(gc->vm, obj_val(string));

    table_set(gc, strings, string, nil_val());

    pop(gc->vm);

    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';

    return string;
}

ObjString* copy_string(GarbageCollector* gc, ObjectStore* store, Table* strings, const char* chars, i32 length)
{
    u32 hash = hash_string(chars, length);
    ObjString* interned = table_find_string(strings, chars, length, hash);

    if (interned) return interned;

    return allocate_string(gc, store, strings, chars, length);
}

ObjUpvalue*  new_upvalue(GarbageCollector* gc, ObjectStore* store, Value* slot)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(gc, ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next     = NULL;
    upvalue->closed   = nil_val();
    return upvalue;
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

ObjString* take_string(GarbageCollector* gc, ObjectStore* store, Table* strings, char* chars, i32 length)
{
    u32 hash = hash_string(chars, length);
    ObjString* interned = table_find_string(strings, chars, length, hash);

    if (interned)
    {
        FREE_ARRAY(gc, char, chars, length + 1);
        return interned;
    }
    
    return allocate_string(gc, store, strings, chars, length);
}

void free_object(GarbageCollector* gc, Obj* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif
    switch (object->type)
    {
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(gc, ObjUpvalue*, closure->upvalues, closure->upvalue_count);
            FREE(gc, ObjClosure, object);
        }
        break;
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)object;
            free_chunk(gc, &function->chunk);
            FREE(gc, ObjFunction, object);
        }
        break;
        case OBJ_NATIVE:
        {
            FREE(gc, ObjNative, object);
        }
        break;
        case OBJ_STRING:
        {
            FREE(gc, ObjString, object);
        }
        break;
        case OBJ_UPVALUE:
        {
            FREE(gc, ObjUpvalue, object);
        }
        break;
    }
}

void print_object(Value value)
{
    switch(value.as.obj->type)
    {
        case OBJ_CLOSURE:
        {
            print_function(as_closure(value)->function);
        }
        break;
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
        case OBJ_UPVALUE:
        {
            printf("upvalue");
        }
        break;
    }
}
