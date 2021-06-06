#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================

#define OBJ_TYPE(value) ((value).as.obj->type)

enum ObjType
{
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
};

struct Obj
{
    ObjType type;
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
b32         is_string(Value);
char*        as_cstring(Value);
ObjClosure*  as_closure(Value value);
ObjFunction* as_function(Value value);
ObjString*   as_string(Value);
ObjString*   copy_string(ObjectStore* store, Table* strings, const char*, i32);
ObjUpvalue*  new_upvalue(ObjectStore* store, Value* slot);
ObjFunction* new_function(ObjectStore* store);
ObjClosure*  new_closure(ObjFunction* function, ObjectStore* store);
ObjNative*   new_native(NativeFn function, NativeArguments arguments, ObjectStore* store);
ObjString*   take_string(ObjectStore*, Table* strings, char*, i32);
void         print_object(Value);
void         free_object(Obj*);
// =================================================================

// =================================================================
// Internal Functions
// =================================================================
#define ALLOCATE_OBJ(type, object_type)                 \
    (type*)allocate_object(store, sizeof(type), (object_type))
// =================================================================

#endif
