#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64 // Maximum number of function call frames at any one time. This is the size of the call stack.
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT) // Maximum stack size for local variables = (FRAMES_MAX * 256)

typedef struct
{
    ObjClosure* closure; // Pointer to the compiled Lox closure object currently being executed.
    uint8_t* ip; // The instruction pointer tracking the current bytecode index inside the function.
    Value* slots; // Pointer into the VM stack where this frame's local variables begin.
} CallFrame;

typedef struct
{
    CallFrame frames[FRAMES_MAX]; // An array of active call frames tracking the nested chain of function invocations.
    int frameCount; // The current number of active call frames sitting inside the frames array.
    Value stack[STACK_MAX]; // VM's stack that is used for local variables within statements/expressions.
    Value* stackTop; // Pointer to the top of the VM stack.
    Table globals; // Hash table storing the global variables defined in a CLox program. It is of the form (<variableName>, <value>).
    Table strings; // Interning table used to store a single unique copy of every string literal in the program.
    ObjString* initString; // String used by the VM to quickly call constructors.
    ObjUpvalue* openUpvalues; // Linked list representing the open up values within the source code.
    size_t bytesAllocated; // The total number of bytes allocated into memory. This can be used to tune the frequency in which the GC is run.
    size_t nextGC; // Threshold that indicates when to run the GC.
    Obj* objects; // Head of the list of dynamically allocated objects within a given CLox program.
    int grayCount; // The total number of grey objects currently pending processing inside the garbage collector's working list stack.
    int grayCapacity; // The maximum capacity limits of the dynamically allocated memory buffer reserved for the garbage collection grey stack.
    Obj** grayStack; // Array of object pointers forming the tracking stack working list used by the tri-colour marking garbage collector.
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
