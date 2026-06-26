#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// When set implement NaN boxing on the value representations, otherwise use the default value representation.
//#define NAN_BOXING

// When set, use the debug module to print out the chunk's byte-code. This is used to debug the compiler.
//#define DEBUG_PRINT_CODE

// Debugging option when writing the VM. When true => VM will disassemble and print the instruction just before execution dynamically.
// This is similar to the chunk debugging which did this statically and in one pass.
//#define DEBUG_TRACE_EXECUTION

// GC Utilities
// Debugging tool to enable the GC at every possible moment.
//#define DEBUG_STRESS_GC

// Produce a log for the GC when dynamic memory operations are performed. This is to aid debugging the GC.
//#define DEBUG_LOG_GC

// Maximum number of local variables that can be in scope at any one time.
#define UINT8_COUNT (UINT8_MAX + 1)

#endif
