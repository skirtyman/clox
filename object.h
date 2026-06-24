#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

// Get the type of a given Obj.
#define OBJ_TYPE(value) (AS_OBJ(value) -> type)

// Check if in Obj is a function/string.
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)   isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)   isObjType(value, OBJ_STRING)

// Convert a CLox Value into either a pointer to a ObjFunction or an ObjString pointer / the character array associated with the string.
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) \
    (((ObjNative*)AS_OBJ(value)) -> function)
#define AS_STRING(value)   ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString*)AS_OBJ(value)) -> chars)

// The range of types that are heap-allocated within CLox
typedef enum
{
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
} ObjType;

// A heap-allocated object within CLox.
struct Obj
{
    ObjType type;
    struct Obj* next; // Store a reference to the next dynamically allocated object. This is so garbage collection is made easier.
};

typedef struct
{
    Obj obj; // Base object state for garbage collection and type tracking. Putting this first,
             // ensures it is first within memory, allowing us to safely cast an ObjFunction* to Obj*.
    int arity; // Number of parameters to the function.
    Chunk chunk; // The chunk of byte-code associated with the function body.
    ObjString* name; // The name of the function, this is useful for reporting runtime errors.
} ObjFunction;

// Define the C function pointer signature used for host-environment native built-in functions.
typedef Value (*NativeFn)(int argCount, Value* args);

// Represents a host-defined native function object wrap inside the Lox runtime environment.
typedef struct
{
    Obj obj; // The base object header tracking garbage collection state and runtime object type identification.
    NativeFn function; // A pointer to the underlying C implementation function to be invoked by the interpreter.
} ObjNative;


// A heap-allocated string within CLox.
struct ObjString
{
    Obj obj; // Base object state for garbage collection and type tracking. Putting this first,
             // ensures it is first within memory, allowing us to safely cast an ObjString* to Obj*.
    int length; // The number of characters in the string, excluding the null character.
    char* chars; // Pointer to the heap-allocated, null-terminated string.
    uint32_t hash; // The hash value used to index into the hash table, storing the strings.
};

// Function Utilities
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);

// String Utilities
// Takes ownership of an existing heap-allocated character array, interns it, and wraps it in an ObjString without allocating new memory for the raw text.
ObjString* takeString(char* chars, int length);
// Allocates a new ObjString on the heap, copies the given character array into it, and null-terminates it.
ObjString* copyString(const char* chars, int length);
// Print an CLox object to the console.
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value) -> type == type;
}

#endif
