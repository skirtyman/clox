#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// Define a type synonym for the values represented in the VM.
typedef double Value;

// Define a dynamic-array of values within the VM. This is the constant pool and allows us to query literal values by index.
typedef struct
{
    int count; // Number of values within the constant pool.
    int capacity;
    Value* values;
} ValueArray;

// Function to initialise a dynamic array storing the constant pool. We use a pointer to modify the chunk directly (pass-by-reference).
void initValueArray(ValueArray* array);
// Function to write a literal value to the constant pool. Ensuring we resize where appropriate.
void writeValueArray(ValueArray* array, Value value);
// This function resets the constant pool, clearing its fields.
void freeValueArray(ValueArray* array);
// Print a value to the standard terminal.
void printValue(Value value);

#endif
