#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
    // If the new size requested is zero, free the memory block and clear the pointer.
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    // Resize the existing dynamic memory block to match the new size specification given in the header file. 0
    void* result = realloc(pointer, newSize);
    // Re-allocating the dynamic array has failed.
    if (result == NULL) exit(1);
    return result;
}

// Free a singular dynamically allocated object.
static void freeObject(Obj* object)
{
    // Inspects the object's type tag to determine the correct cleanup logic.
    switch(object -> type)
    {
        case OBJ_STRING:
        {
            // Safely casts the base Obj pointer back to its specific ObjString type.
            ObjString* string = (ObjString*)object;

            // Frees the heap-allocated character array, including its null terminator.
            FREE_ARRAY(char, string -> chars, string -> length + 1);

            // Frees the ObjString structure itself from heap memory.
            FREE(ObjString, object);
            break;
        }
    }
}

// Free all dynamically allocated objects stored within the VM linked list.
void freeObjects()
{
    Obj* object = vm.objects;
    while (object != NULL)
    {
        Obj* next = object -> next;
        freeObject(object);
        object = next;
    }
}
