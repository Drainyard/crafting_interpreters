#ifndef CLOX_TABLE_H
#define CLOX_TABLE_H

// =================================================================
// API
// =================================================================

// =================================================================
// Types
// =================================================================

struct Entry
{
    ObjString* key;
    Value value;
};

struct Table
{
    i32 count;
    i32 capacity;
    Entry* entries;
};

// =================================================================

// =================================================================
// API Functions
// =================================================================
void init_table(Table* table);
void free_table(Table* table);
bool table_set(Table* table, ObjString* key, Value value);
bool table_delete(Table* table, ObjString* key);
void table_add_all(Table* from, Table* to);
ObjString* table_find_string(Table* table, const char* chars, i32 length, u32 hash);
bool table_get(Table* table, ObjString* key, Value* value);
// =================================================================

#endif
