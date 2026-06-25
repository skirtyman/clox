#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

// Convert a string containing Lox source code into a sequence of chunks of byte code to be stored in `chunk` (pass-by-reference to a local chunk within the VM).
// Return the compiled function to the interpreter.
ObjFunction* compile(const char* source);
// Mark the roots produced by the compiler, to be swept by the GC.
void markCompilerRoots();


#endif
