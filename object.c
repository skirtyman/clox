#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type)
{
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object -> type = type;

    // When allocating a new object, add it to the list within the VM.
    object -> next = vm.objects;
    vm.objects = object;
    return object;
}

ObjFunction* newFunction()
{
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function -> arity = 0;
    function -> name = NULL;
    initChunk(&function -> chunk);
    return function;
}

ObjNative* newNative(NativeFn function)
{
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native -> function = function;
    return native;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash)
{
    // Allocates an ObjString on the heap and initializes its base Obj metadata as a string type.
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);

    // Set the characteristics of the string.
    string -> length = length;
    string -> chars = chars;
    string -> hash = hash;
    // Default to interning every string, we do not need values and can therefore set all values to NULL.
    tableSet(&vm.strings, string, NIL_VAL);
    return string;
}

// Hash function to compute the hashes of dynamically allocated strings.
static uint32_t hashString(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 1677619;
    }
    return hash;
}

// Takes ownership of an existing heap-allocated character array, interns it, and wraps it in an ObjString without allocating new memory for the raw text.
ObjString* takeString(char* chars, int length)
{
    // Computes the hash code for the provided raw character array.
    uint32_t hash = hashString(chars, length);

    // Checks the global interning table to see if an identical string already exists.
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL)
    {
        // Frees the redundant duplicate character array since we are reusing the interned one.
        FREE_ARRAY(char, chars, length + 1);

        // Returns the pre-existing interned string pointer to save memory.
        return interned;
    }

    // Wraps the unique character array in a new object structure if it was not found in the table.
    return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length)
{
    // Compute the hash of the string.
    uint32_t hash = hashString(chars, length);

    // Checks if an identical string has already been created and stored in the global table.
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    // Returns the existing string pointer immediately if found to prevent duplicate allocations.
    if (interned != NULL) return interned;

    // Allocate an array on the heap for the string, allocating 1 extra byte for null-termination.
    char* heapChars = ALLOCATE(char, length + 1);
    // Copy the character array into the allocated array on the heap.
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    // Wrap the heap-allocated array in an ObjString for use within the VM.
    return allocateString(heapChars, length, hash);
}

// Print a function by showing its name.
static void printFunction(ObjFunction* function)
{
    if (function -> name == NULL)
    {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function -> name -> chars);
}

void printObject(Value value)
{
    // Switch over the type of the object.
    switch(OBJ_TYPE(value))
    {
        case OBJ_FUNCTION: // Found a string, therefore get its name and print.
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STRING: // Found a string, therefore get its character array and print.
            printf("%s", AS_CSTRING(value));
            break;
    }
}
