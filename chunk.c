#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk)
{
    chunk -> count = 0;
    chunk -> capacity = 0;
    chunk -> code = NULL;
}

void writeChunk(Chunk* chunk, uint8_t byte)
{
    // Check that the chunk has capacity to store another op-code.
    if (chunk -> capacity < chunk -> count + 1)
    {
        // Array is too small so resize.
        int oldCapacity = chunk -> capacity;
        // Change the capacity of the chunk and re-allocate the old chunk of memory to the new (re-sized) array.
        chunk -> capacity = GROW_CAPACITY(oldCapacity);
        chunk -> code = GROW_ARRAY(uint8_t, chunk -> code, oldCapacity, chunk -> capacity);
    }
    // Add the new op-code to the dynamic array `chunk`.
    chunk -> code[chunk -> count] = byte;
    chunk -> count++;
}

void freeChunk(Chunk* chunk)
{
    // Free the memory allocated to the dynamic array storing the byte-code of the chunk to be freed.
    FREE_ARRAY(uint8_t, chunk -> code, chunk -> capacity);
    // Re-initialise the same chunk with default parameters. Hence, resetting it.
    initChunk(chunk);
}
