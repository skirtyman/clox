#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

typedef enum
{
    OP_RETURN, // The VM has reached the end of a chunk of byte-code and returns the current execution frame (function).
} OpCode;

typedef struct
{
    int count;
    int capacity;
    uint8_t* code;
} Chunk; // Represents a given chunk of byte-code as a dynamically sized array of bytes (the op-codes).

// Function to initialise a dynamic array storing a chunk of byte-code. We use a pointer to modify the chunk directly (pass-by-reference).
void initChunk(Chunk* chunk);
// This function resets a given chunk, freeing the memory allocated to its fields.
void freeChunk(Chunk* chunk);
// Function to write a byte-code instruction to the dynamic array. Ensuring we resize where appropriate.
void writeChunk(Chunk* chunk, uint8_t byte);


#endif
