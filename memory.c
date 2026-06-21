#include <stdlib.h>

#include "memory.h"

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
