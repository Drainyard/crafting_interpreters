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
    OBJ_STRING
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
bool is_string(Value);
char* as_cstring(Value);
ObjString* as_string(Value);
ObjString* copy_string(ObjectStore*, Table* strings, const char*, i32);
ObjString* take_string(ObjectStore*, Table* strings, char*, i32);
void print_object(Value);
void free_object(Obj*);
// =================================================================

// =================================================================
// Internal Functions
// =================================================================
#define ALLOCATE_OBJ(type, object_type)                 \
    (type*)allocate_object(store, sizeof(type), (object_type))
// =================================================================

#endif
