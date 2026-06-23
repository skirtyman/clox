#include <stdarg.h>
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

// Report a runtime error to the use.
static void runtimeError(const char* format, ...)
{
    // Initialize a variable argument list to handle dynamic string formatting.
    va_list args;
    va_start(args, format);
    // Print the formatted error message directly to the standard error stream.
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // Calculate the current instruction index by finding the offset of the instruction pointer
    // relative to the beginning of the byte-code chunk. We subtract 1 because `ip` has already
    // advanced past the failing instruction.
    size_t instruction = vm.ip - vm.chunk->code - 1;
    // Look up the source code line number associated with the failing byte-code instruction offset.
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    // Clear the VM's value stack to reset the engine state cleanly after the crash.
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

// Peek the item <distance> from the the top of the stack.
// Distance == 0 => Normal peek, Distance == 1 => peek() after popping an item from the stack.
static Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

// Determine the falsiness of a value. Nil and False => falsey and every other value is truthy.
static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
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
    #define BINARY_OP(valueType, op) \
        do { \
            if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
                runtimeError("Operands must be numbers."); \
                return INTERPRET_RUNTIME_ERROR; \
            } \
            double b = AS_NUMBER(pop()); \
            double a = AS_NUMBER(pop()); \
            push(valueType(a op b)); \
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
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL:
            {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: BINARY_OP(NUMBER_VAL, +); break;
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                // Check that value to be negative is numeric and hence negatable. Return a runtime error if not.
                if (!IS_NUMBER(peek(0)))
                {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
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

InterpretResult interpret(const char* source)
{
    // Initialize a temporary, local chunk to store the compiled byte-code for the VM to execute.
    Chunk chunk;
    initChunk(&chunk);

    // Compile the source code into byte-code. Abort and clean up memory if a compilation error occurs.
    if (!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    // Bind the compiled source code to the VM ready for execution. Also set IP to point to the first compiled instruction.
    vm.chunk = &chunk;
    vm.ip = vm.chunk -> code;

    // Execute the byte-code within the virtual machine.
    InterpretResult result = run();

    // Free the allocated byte-code in the chunk and return the result from the VM.
    freeChunk(&chunk);
    return result;
}
