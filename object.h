#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// Get the type of a given Obj.
#define OBJ_TYPE(value) (AS_OBJ(value) -> type)

// Check if in Obj is a string.
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Convert a CLox Value into either an ObjString pointer or the character array within the string.
#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value)) -> chars)

// The range of types that are heap-allocated within CLox
typedef enum
{
    OBJ_STRING,
} ObjType;

// A heap-allocated object within CLox.
struct Obj
{
    ObjType type;
    struct Obj* next; // Store a reference to the next dynamically allocated object. This is so garbage collection is made easier.
};

// A heap-allocated string within CLox.
struct ObjString
{
    Obj obj; // Base object state for garbage collection and type tracking. Putting this first,
             // ensures it is first within memory, allowing us to safely cast an ObjString* to Obj*.
    int length; // The number of characters in the string, excluding the null character.
    char* chars; // Pointer to the heap-allocated, null-terminated string.
    uint32_t hash; // The hash value used to index into the hash table, storing the strings.
};

// Allocates a new ObjString on the heap, copies the given character array into it, and null-terminates it.
ObjString* copyString(const char* chars, int length);
// Print an CLox object to the console.
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value) -> type == type;
}

#endif
