#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT, // Form >>> OP_CONSTANT <index> => VM must fetch the constant at <index> within the chunk's constant pool. This is a 2 byte instruction as index is 1-byte in size.
    OP_ADD, // Form >>> OP_ADD => Returns the result of arithmetic addition of the 2 operands. This is a 1 byte instruction as OP_ADD, uses the operands stored in the stack and not
            //                    for the instruction itself. The same applies to OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE.
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE, // Form >>> OP_NEGATE <operand> => Returns negation of operand => OP_NEGATE 1.0 == -1.0. This is a 2 byte instruction.
    OP_RETURN, // The VM has reached the end of a chunk of byte-code and returns the current execution frame (function).
} OpCode;

typedef struct
{
    int count; // Number of values within the constant pool.
    int capacity;
    uint8_t* code;
    int* lines; // Store a list of line numbers where each number is the line that a byte-code instruction belongs to.
    ValueArray constants; // Each chunk of byte-code contains an associated constant pool, allowing each chunk to have its own set of constants /
                          // not be limited by the size of the array.
} Chunk; // Represents a given chunk of byte-code as a dynamically sized array of bytes (the op-codes).

// Function to initialise a dynamic array storing a chunk of byte-code. We use a pointer to modify the chunk directly (pass-by-reference).
void initChunk(Chunk* chunk);

// This function resets a given chunk, freeing the memory allocated to its fields.
void freeChunk(Chunk* chunk);

// Function to write a byte-code instruction to the dynamic array including the line number it belongs to. Ensuring we resize where appropriate.
void writeChunk(Chunk* chunk, uint8_t byte, int line);

// Add a constant to the constant pool of the chunk.
int addConstant(Chunk* chunk, Value value);

#endif
