#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256 // Maximum size for a sequence of instructions.


typedef struct
{
    Chunk* chunk; // The chunk to be executed by the virtual machine.
    uint8_t* ip; // The instruction pointer. It stores the current location within the chunk's code that the VM is executing.
    Value stack[STACK_MAX]; // VM's stack that is used for local variables within statements/expressions.
    Value* stackTop; // Pointer to the top of the VM stack.
    Table globals; // Hash table storing the global variables defined in a CLox program. It is of the form (<variableName>, <value>).
    Table strings; // Interning table used to store a single unique copy of every string literal in the program.
    Obj* objects; // Head of the list of dynamically allocated objects within a given CLox program.
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();

// Interpret a given string of Lox source code, returning a status code for the result of the interpreter.
InterpretResult interpret(const char* chunk);

// Stack operations to be able to manipulate the VM's instruction stack.
void push(Value value);
Value pop();

#endif
