#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

// Create a global instance of the virtual machine. This is not good practice as it reduces flexibility and ease of use within host applications.
VM vm;

static void resetStack()
{
    vm.stackTop = vm.stack;
}

void initVM()
{
    resetStack();
}


void freeVM()
{
    resetStack();
}

void push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}

static InterpretResult run()
{
    // Get a byte-code instruction by dereferencing the pointer and returning the item and then incrementing the instruction pointer.
    // This is so IP always points to the address of the next instruction.
    #define READ_BYTE() (*vm.ip++)
    // The next byte in the chunk at a OP_CONSTANT instruction is the address within the constant pool, therefore we fetch the literal value
    // at this address.
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    // Stack operations required to process binary operations.
    #define BINARY_OP(op) \
        do { \
          double b = pop(); \
          double a = pop(); \
          push(a op b); \
        } while (false)


    // Pull instructions from the chunk until a return statement is found and the loop is broken.
    for (;;)
    {
        #ifdef DEBUG_TRACE_EXECUTION
            // Print the contents of the VM's instruction stack to the console.
            printf("          ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++)
            {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }
            printf("\n");
            disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk -> code)); // Display the instruction at the instruction pointer within the chunk.
                                                                               // We use pointer arithmetic as vm.ip is an absolute address in the chunk.
        #endif

        uint8_t instruction;
        // Fetch the instruction from the chunk and then execute it according to the type of the fetched instruction.
        switch (instruction = READ_BYTE())
        {
            case OP_CONSTANT:
            {
                Value constant = READ_CONSTANT();
                push(constant); // Constant found, add to VM's instruction stack.
                break;
            }
            case OP_ADD: BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE: BINARY_OP(/); break;
            case OP_NEGATE:
                push(-pop());
                break;
            case OP_RETURN:
                // Interpreter has successfully finished executing the chunk, we can know print the top of the stack (the result).
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
        }
    }
    // Delete MACROs to not interfere with the rest of the interpreter.
    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef BINARY_OP
}

InterpretResult interpret(const char* chunk)
{
    compile(source);
    return INTERPRET_OK;
}
