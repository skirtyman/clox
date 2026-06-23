#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// When set, use the debug module to print out the chunk's byte-code. This is used to debug the compiler.
#define DEBUG_PRINT_CODE
// Debugging option when writing the VM. When true => VM will disassemble and print the instruction just before execution dynamically.
// This is similar to the chunk debugging which did this statically and in one pass.
#define DEBUG_TRACE_EXECUTION

#endif
