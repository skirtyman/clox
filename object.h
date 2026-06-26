#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

// Get the type of a given Obj.
#define OBJ_TYPE(value) (AS_OBJ(value) -> type)

// Check if in Obj is a class/closure/function/instance/string.
#define IS_BOUND_METHOD(value)    isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)           isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)         isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)        isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)        isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)          isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)          isObjType(value, OBJ_STRING)

// Convert a CLox Value into its respective runtime C implementation within the VM.
#define AS_BOUND_METHOD(value)    ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)    ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)  ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value) \
    (((ObjNative*)AS_OBJ(value)) -> function)
#define AS_STRING(value)   ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString*)AS_OBJ(value)) -> chars)

// The range of types that are heap-allocated within CLox
typedef enum
{
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
} ObjType;

// A heap-allocated object within CLox.
struct Obj
{
    ObjType type;
    bool isMarked; // Mark to determine whether an object should be swept by the GC.
    struct Obj* next; // Store a reference to the next dynamically allocated object. This is so garbage collection is made easier.
};

typedef struct
{
    Obj obj; // Base object state for garbage collection and type tracking. Putting this first,
             // ensures it is first within memory, allowing us to safely cast an ObjFunction* to Obj*.
    int arity; // Number of parameters to the function.
    int upvalueCount; // The number of Upvalues captured within the closure.
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

typedef struct ObjUpvalue
{
    Obj obj; // The base object header tracking garbage collection meta-data and type identification for this heap-allocated upvalue.
    Value* location; // A direct pointer to the storage location of the closed-over variable, pointing to either the VM stack or this object's own heap storage.
    Value closed; // Store the index within the heap that the closed upvalue points to.
    struct ObjUpvalue* next; // Create a linked list of upvalues to ensure all variables point to the same upvalues and not copies within different memory locations.
} ObjUpvalue;

// A heap-allocated closure within CLox.
typedef struct
{
    Obj obj; // Base object state for garbage collection and type tracking. Putting this first,
             // ensures it is first within memory, allowing us to safely cast an ObjClosure* to Obj*.
    ObjFunction* function; // Pointer to the underlying compiled function that this closure wraps.
    ObjUpvalue** upvalues; // Dynamic array of upvalues associated with any particular closure.
                           // EXTRACT FROM 25.3.1: "Different closures may have different numbers of upvalues, so we need a dynamic array. The upvalues themselves are
                           //                       dynamically allocated too, so we end up with a double pointer—a pointer to a dynamically allocated array of pointers
                           //                       to upvalues. We also store the number of elements in the array."
    int upvalueCount; // The number of upvalues within the closure.
} ObjClosure;

// A heap-allocated class within CLox. This is its runtime representation within the VM.
typedef struct
{
    Obj obj; // Base object state for garbage collection and type tracking.
    ObjString* name; // The name of the class.
    Table methods; // Hash table storing the methods associated with the class. Where keys are the method name
                   // and values are ObjClosure which represent the method body.
} ObjClass;

// Runtime representation of an instance of a class.
typedef struct
{
    Obj obj; // Base object state for garbage collection and type tracking.
    ObjClass* klass; // Class in which this instance is a member of.
    Table fields; // The fields and values of those fields within the instance.
} ObjInstance;

// Runtime representation of a method bound to an instance, resolving 'this' dynamically.
typedef struct
{
    Obj obj; // Base object state for garbage collection and type tracking.
    Value reciever; // The instance ('this') to which the method is bound.
    ObjClosure* method; // The actual closure function that implements the method.
} ObjBoundMethod;

// Function Utilities
ObjBoundMethod* newBoundMethod(Value reciever, ObjClosure* method);
ObjClass* newClass(ObjString* name);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn function);


// String Utilities
// Takes ownership of an existing heap-allocated character array, interns it, and wraps it in an ObjString without allocating new memory for the raw text.
ObjString* takeString(char* chars, int length);
// Allocates a new ObjString on the heap, copies the given character array into it, and null-terminates it.
ObjString* copyString(const char* chars, int length);
// Utility to create a runtime representation of an upvalue.
ObjUpvalue* newUpvalue(Value* slot);
// Print an CLox object to the console.
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value) -> type == type;
}

#endif
