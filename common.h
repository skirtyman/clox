#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Debugging option when writing the VM. When true => VM will disassemble and print the instruction just before execution dynamically.
// This is similar to the chunk debugging which did this statically and in one pass.
#define DEBUG_TRACE_EXECUTION

#endif
