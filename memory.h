#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

// Re-allocates the small dynamic array `Chunk` into a new size one.
// Void pointers are used to resize arrays of different types. The behaviour of this function is defined in the comment below:
/*
oldSize	    newSize	                Operation
0	        Non-zero	            Allocate new block.
Non-zero	0	                    Free allocation.
Non-zero	Smaller than oldSize	Shrink existing allocation.
Non-zero	Larger than oldSize	    Grow existing allocation.
*/
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif
