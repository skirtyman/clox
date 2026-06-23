#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

// Convert a string containing Lox source code into a sequence of chunks of byte code to be stored in `chunk` (pass-by-reference to a local chunk within the VM).
bool compile(const char* source, Chunk* chunk);

#endif
