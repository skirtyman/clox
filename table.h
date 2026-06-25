#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct
{
    ObjString* key;
    Value value;
} Entry;

typedef struct
{
    int count;       // The total number of active key-value pairs currently stored in the table.
    int capacity;    // The total number of allocated slots (buckets) available in the entries array. NOTE: Load factors = count / capacity.
    Entry* entries;  // Pointer to a dynamically allocated array of Entry structures (the buckets).
} Table;

// Initialise and free hash tables.
void initTable(Table* table);
void freeTable(Table* table);

// Add, get and delete key-value pairs into the hash table.
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
// Copying all entries of one hash table to another.
void tableAddAll(Table* from, Table* to);

// Find a string in a hash table using string interning and hence safe `==`.
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
// Mark all of the entries within the hash table to be swept by the GC.
void markTable(Table* table);

#endif
