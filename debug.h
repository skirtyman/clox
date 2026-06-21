#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

// Decode and print every byte-code instruction inside a dynamic array `chunk` to the console.
// Name represents a label we are giving to the chunk being printed (disassembled).
void disassembleChunk(Chunk* chunk, const char* name);
// Decode and print a single byte-code instruction at a specific index within the chunk.
// We can use the OP-CODE type (in chunk.h) to be able to find the correct instruction name.
int disassembleInstruction(Chunk* chunk, int offset);

#endif
