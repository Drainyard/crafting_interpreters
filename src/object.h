#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================

#define OBJ_TYPE(value) ((value).as.obj->type)

#define IS_STRING(value) (is_obj_type(value, OBJ_STRING))
#define IS_INSTANCE(value) (is_obj_type(value, OBJ_INSTANCE))
#define IS_NATIVE(value) (is_obj_type(value, OBJ_NATIVE))
#define IS_FUNCTION(value) (is_obj_type(value, OBJ_FUNCTION))
#define IS_CLOSURE(value) (is_obj_type(value, OBJ_CLOSURE))
#define IS_BOUND_METHOD(value) (is_obj_type(value, OBJ_BOUND_METHOD))
#define IS_CLASS(value) (is_obj_type(value, OBJ_CLOSURE))
#define IS_UPVALUE(value) (is_obj_type(value, OBJ_UPVALUE))

#define AS_OBJ(value)       (value.as.obj)
#define AS_OBJ_TYPE(value, type) ((type*)AS_OBJ(value))
#define AS_NATIVE(value)       (AS_OBJ_TYPE(value, ObjNative))
#define AS_FUNCTION(value)     (AS_OBJ_TYPE(value, ObjFunction))
#define AS_CLOSURE(value)      (AS_OBJ_TYPE(value, ObjClosure))
#define AS_BOUND_METHOD(value) (AS_OBJ_TYPE(value, ObjBoundMethod))
#define AS_CLASS(value)        (AS_OBJ_TYPE(value, ObjClass))
#define AS_INSTANCE(value)     (AS_OBJ_TYPE(value, ObjInstance))
#define AS_CSTRING(value)      (AS_OBJ_TYPE(value, ObjString)->chars)
#define AS_STRING(value)       (AS_OBJ_TYPE(value, ObjString))
#define AS_UPVALUE(value)      (AS_OBJ_TYPE(value, ObjUpvalue))

enum ObjType
{
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
};

struct Obj
{
    ObjType type;
    b32 is_marked;
    Obj* next;
};

struct ObjString
{
    Obj obj;
    i32 length;
    u32 hash;

    // Leave at bottom for flexible array
    char chars[1];
};

struct ObjUpvalue
{
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
};

struct ObjFunction
{
    Obj obj;
    i32 arity;
    i32 upvalue_count;
    Chunk chunk;
    ObjString* name;
};

struct ObjClosure
{
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    i32 upvalue_count;
};

struct ObjClass
{
    Obj obj;
    ObjString* name;
    Table methods;
};

struct ObjInstance
{
    Obj obj;
    ObjClass* klass; //EEK
    Table  fields;
};

struct ObjBoundMethod
{
    Obj obj;
    Value receiver;
    ObjClosure* method;
};

#define MAX_ARITY 8
struct NativeArguments
{
    i32 arity;
    ValueType types[MAX_ARITY];
};

using NativeFn = Value(*)(i32 arg_count, Value* args);
struct ObjNative
{
    Obj obj;
    /* ValueType argument_types[8]; */
    NativeArguments arguments;
    /* i32 arity; */
    NativeFn function;
};

struct ObjectStore
{
    Obj* objects;
};

struct VM;
struct Table;

// =================================================================

// =================================================================
// API Functions
// =================================================================
ObjString*      copy_string(GarbageCollector* gc, ObjectStore* store, Table* strings, const char*, i32);
ObjUpvalue*     new_upvalue(GarbageCollector* gc, ObjectStore* store, Value* slot);
ObjFunction*    new_function(GarbageCollector* gc, ObjectStore* store);
ObjInstance*    new_instance(GarbageCollector* gc, ObjectStore* store, ObjClass* klass);
ObjBoundMethod* new_bound_method(GarbageCollector* gc, ObjectStore* store, Value receiver, ObjClosure* method);
ObjClass*       new_class(GarbageCollector* gc, ObjectStore* store, ObjString* name);
ObjClosure*     new_closure(GarbageCollector* gc, ObjFunction* function, ObjectStore* store);
ObjNative*      new_native(GarbageCollector* gc, NativeFn function, NativeArguments arguments, ObjectStore* store);
ObjString*      take_string(GarbageCollector* gc, ObjectStore*, Table* strings, char*, i32);
void            print_object(Value);
void            free_object(GarbageCollector* gc, Obj*);
// =================================================================

// =================================================================
// Internal Functions
// =================================================================
#define ALLOCATE_OBJ(gc, type, object_type)                          \
    (type*)allocate_object(gc, store, sizeof(type), (object_type))
// =================================================================

#endif
