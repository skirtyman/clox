#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256 // Maximum size for a sequence of instructions.


typedef struct
{
    Chunk* chunk; // The chunk to be executed by the virtual machine.
    uint8_t* ip; // The instruction pointer. It stores the current location within the chunk's code that the VM is executing.
    Value stack[STACK_MAX]; // VM's stack that is used for local variables within statements/expressions.
    Value* stackTop;
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
// Interpret a given chunk of byte code returning a status code for the result of the interpreter.
InterpretResult interpret(Chunk* chunk);
// Stack operations to be able to manipulate the VM's instruction stack.
void push(Value value);
Value pop();

#endif
