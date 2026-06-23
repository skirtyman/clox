#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// Forward definitions for the heap-allocated objects within CLox. This is to avoid circular dependency with #include between `value.h` and `object.h`
typedef struct Obj Obj;
typedef struct ObjString ObjString;

// Define an enum to represent data within the VM.
typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ // Any item stored on the stack that is not a number, boolean, or nil, such as strings/functions/classes etc.
            // In this case VAL_OBJ acts as a pointer to the heap which stores the object itself.
} ValueType;

// Define a tagged union to compactly represent values within the VM.
typedef struct
{
    ValueType type; // The type a particular value has.
    union // The range of values the data bits can be used to represent.
    {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

// MACROs to check a CLox Value has a particular type, to ensure the casting MACROs is type safe.
#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value)    ((value).type == VAL_OBJ)

// MACROs to convert a CLox Value into a native C value.
#define AS_OBJ(value)    ((value).as.obj)
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

// MACROs to convert a native C value into a CLox Value.
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

// Define a dynamic-array of values within the VM. This is the constant pool and allows us to query literal values by index.
typedef struct
{
    int count; // Number of values within the constant pool.
    int capacity;
    Value* values;
} ValueArray;

// Function to determine if two CLox values are equal.
bool valuesEqual(Value a, Value b);
// Function to initialise a dynamic array storing the constant pool. We use a pointer to modify the chunk directly (pass-by-reference).
void initValueArray(ValueArray* array);
// Function to write a literal value to the constant pool. Ensuring we resize where appropriate.
void writeValueArray(ValueArray* array, Value value);
// This function resets the constant pool, clearing its fields.
void freeValueArray(ValueArray* array);
// Print a value to the standard terminal.
void printValue(Value value);

#endif
