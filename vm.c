#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

// Create a global instance of the virtual machine. This is not good practice as it reduces flexibility and ease of use within host applications.
VM vm;

static Value clockNative(int argCount, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void resetStack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

// Report a runtime error to the user and display a stack trace for debugging.
static void runtimeError(const char* format, ...)
{
    // Initialize a variable argument list to handle dynamic string formatting.
    va_list args;
    va_start(args, format);
    // Print the formatted error message directly to the standard error stream.
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // Walk backward from the topmost active frame down to the entry-level script frame.
    for (int i = vm.frameCount - 1; i >= 0; i--)
    {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame -> closure -> function;
        // Calculate the executing instruction index by looking one byte behind the forward-pointing instruction pointer.
        size_t instruction = frame -> ip - function -> chunk.code - 1;
        // Print the source code line number stored in the current chunk's debug array.
        fprintf(stderr, "[line %d] in ", function -> chunk.lines[instruction]);
        // If the function name pointer is null, we are executing the top-level implicit main script block.
        if (function -> name == NULL)
        {
            fprintf(stderr, "script\n");
        }
        else
        {
            // Print the human-readable string representation of the named function declaration.
            fprintf(stderr, "%s\n", function -> name -> chars);
        }
    }
    resetStack();
}

static void defineNative(const char* name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM()
{
    resetStack();
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);

    defineNative("clock", clockNative);
}


void freeVM()
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
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

// Initialize a new CallFrame on the execution stack to run a compiled closure.
static bool call(ObjClosure* closure, int argCount)
{
    // Check the function has been called with the correct number of arguments.
    if (argCount != closure -> function -> arity)
    {
        runtimeError("Expected %d arguments but got %d.", closure -> function -> arity, argCount);
        return false;
    }

    // Ensure that a closure call chain does not overflow the call stack. Hence report an error.
    if (vm.frameCount == FRAMES_MAX)
    {
        runtimeError("Stack Overflow.");
        return false;
    }

    // Allocate and slide into a fresh execution frame slot at the top of the VM's call stack.
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame -> closure = closure;

    // Anchor the frame's instruction pointer directly to the beginning of the function's bytecode chunk.
    frame -> ip = closure -> function -> chunk.code;

    // Calculate the frame's window into the value stack, positioning slot 0 to point to the function object itself.
    // This allows parameters and locals to be indexed relative to this baseline offset.
    frame -> slots = vm.stackTop - argCount - 1;
    return true;
}


// Validate and route a call expression based on the runtime type of the callee object.
static bool callValue(Value callee, int argCount)
{
    // Ensure the callee is a heap-allocated object before inspecting its internal type tag.
    if (IS_OBJ(callee))
    {
        switch (OBJ_TYPE(callee))
        {
            // We do not need to consider OBJ_FUNCTION as we assume all functions will be wrapped in a closure (even if it will never be used).
            // Therefore the VM never needs to execute a bare function.
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE:
            {
                // Cast the generic value into a concrete native function object pointer, then extract its raw C function pointer.
                NativeFn native = AS_NATIVE(callee);
                // Directly invoke the C function, passing the argument count and a pointer to the first argument slot on the VM stack.
                Value result = native(argCount, vm.stackTop - argCount);
                // Discard the arguments and the native function object from the stack by sliding the top pointer backward.
                vm.stackTop -= argCount + 1;
                // Place the returned value from the host environment's execution back onto the top of the stack.
                push(result);
                return true;
            }
            default:
                break; // Non-callable object type.
        }
    }

    // Fall-through case handling primitive values or non-invokable objects like strings or instances.
    runtimeError("Can only call functions and classes.");
    return false;
}

static ObjUpvalue* captureUpvalue(Value* local)
{
    // Tracks the immediately preceding upvalue node in the sorted linked list during iteration.
    ObjUpvalue* prevUpvalue = NULL;
    // Start scanning from the head of the VM's global linked list of open upvalues.
    ObjUpvalue* upvalue = vm.openUpvalues;
    // Walk down the linked list as long as we haven't reached the end and the current upvalue points to a higher stack address.
    while (upvalue != NULL && upvalue -> location > local)
    {
        prevUpvalue = upvalue;
        upvalue = upvalue -> next;
    }

    // If an existing open upvalue matches the target stack memory address exactly, reuse it to avoid duplicate wrapper objects.
    if (upvalue != NULL && upvalue -> location == local)
    {
        return upvalue;
    }

    // Instantiate a new heap-allocated upvalue object that references the memory address of the target local variable.
    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue -> next = upvalue;

    if (prevUpvalue == NULL)
    {
        vm.openUpvalues = createdUpvalue;
    }
    else
    {
        prevUpvalue -> next = createdUpvalue;
    }

    // Return the newly created upvalue instance to be stored inside the current closure's upvalue array.
    return createdUpvalue;
}

static void closeUpvalues(Value* last)
{
    while (vm.openUpvalues != NULL && vm.openUpvalues -> location >= last)
    {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue -> closed = *upvalue -> location;
        upvalue -> location = &upvalue -> closed;
        vm.openUpvalues = upvalue -> next;
    }
}

// Determine the falsiness of a value. Nil and False => falsey and every other value is truthy.
static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// Concatenate two strings and push them onto the VM stack.
static void concatenate()
{
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a -> length + b -> length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a -> chars, a -> length);
    memcpy(chars + a -> length, b -> chars, b -> length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run()
{
    // Get the call frame being executed by the VM.
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

    // Get a byte-code instruction by dereferencing the pointer and returning the item and then incrementing the instruction pointer.
    // This is so IP always points to the address of the next instruction.
    #define READ_BYTE() (*frame -> ip++)
    // The next byte in the frame at a OP_CONSTANT instruction is the address within the constant pool, therefore we fetch the literal value
    // at this address.
    #define READ_CONSTANT() \
        (frame -> closure -> function -> chunk.constants.values[READ_BYTE()])
    // Read a 16-bit operand from the chunk of byte-code.
    #define READ_SHORT() \
        (frame -> ip += 2, \
        (uint16_t)((frame -> ip[-2] << 8) | frame -> ip[-1]))
    // Read a string literal
    #define READ_STRING() AS_STRING(READ_CONSTANT())
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
        /*
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
            // Display the instruction at the instruction pointer within the chunk.
            // We use pointer arithmetic as vm.ip is an absolute address in the chunk.
            disassembleInstruction(&frame -> closure -> function -> chunk, (int)(frame -> ip - frame -> closure -> function -> chunk.code));
        #endif
        */
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
            case OP_POP: pop(); break;
            case OP_GET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                push(frame -> slots[slot]);
                break;
            }
            case OP_SET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                frame -> slots[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: // Get the value of a global variable within a statement.
            {
                ObjString* name = READ_STRING();  // An index into the constant table is supplied, which points to the string name of a global variable. We can get the global
                                                  // by getting this key and sending it to the heap-allocated hash table `globals` which stores the variables value.
                Value value;
                if (!tableGet(&vm.globals, name, &value)) // If we cannot get a data item, then the global is not defined and hence a runtime error has occurred.
                {
                    runtimeError("Undefined variable '%s'.", name -> chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value); // Value has been found and we can therefore push it onto the stack.
                break;
            }
            case OP_DEFINE_GLOBAL: // Define a global variable by getting the name of the variable from the constant table.
                                   // Taking the value from the top of the stack and store it in a hash table with that name as the key.
            {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL:
            {
                ObjString* name = READ_STRING(); // Read the name the of the global variable being accessed.
                if (tableSet(&vm.globals, name, peek(0))) // If the variable cannot be set, remove it from the table and report a runtime error.
                {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name -> chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_UPVALUE:
            {
                uint8_t slot = READ_BYTE();
                push(*frame -> closure -> upvalues[slot] -> location);
                break;
            }
            case OP_SET_UPVALUE:
            {
                uint8_t slot = READ_BYTE();
                *frame -> closure -> upvalues[slot] -> location = peek(0);
                break;
            }
            case OP_EQUAL:
            {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: // (+) is either addition or concatenation, we can determine the difference between the intended operation by examining types.
            {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
                {
                    concatenate();
                }
                else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
                {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                }
                else
                {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
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
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                frame -> ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) frame -> ip += offset;
                break;
            }
            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                frame -> ip -= offset;
                break;
            }
            case OP_CALL:
            {
                int argCount = READ_BYTE();
                // Pass the arguments and peek down past them to find the callable object slot on the VM stack.
                // If the object is not a valid callable type, trigger a runtime error and halt execution.
                if (!callValue(peek(argCount), argCount))
                {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLOSURE:
            {
                // Read the constant pool index from the bytecode stream and convert the retrieved value into an internal function object pointer.
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                // Instantiate a new heap-allocated closure object that wraps around the target function layout.
                ObjClosure* closure = newClosure(function);
                // Push the newly instantiated closure object onto the VM stack to protect it from garbage collection and prepare it for invocation.
                push(OBJ_VAL(closure));
                // Loop through every upvalue required by the compiled function to populate the runtime closure environment.
                for (int i = 0; i < closure -> upvalueCount; i++)
                {
                    // Read the 1-byte flag indicating whether the upvalue is a local variable in the immediate outer scope.
                    uint8_t isLocal = READ_BYTE();
                    // Read the 1-byte index that specifies the relative storage location of the target variable.
                    uint8_t index = READ_BYTE();
                    if (isLocal)
                    {
                        // Capture a local variable from the current call frame's stack slot range, reusing or creating an upvalue object.
                        closure -> upvalues[i] = captureUpvalue(frame -> slots + index);
                    }
                    else
                    {
                        // Inherit an existing upvalue directly from the current active closure's own upvalue tracking table.
                        closure -> upvalues[i] = frame -> closure -> upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            case OP_RETURN:
            {
                Value result = pop();
                closeUpvalues(frame -> slots);
                vm.frameCount--;
                if (vm.frameCount == 0)
                {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame -> slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
        }
    }
    // Delete MACROs to not interfere with the rest of the interpreter.
    #undef READ_BYTE
    #undef READ_SHORT
    #undef READ_CONSTANT
    #undef READ_STRING
    #undef BINARY_OP
}

InterpretResult interpret(const char* source)
{
    // Compile source code into a top-level implicit function object
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR; // The compiler could not compile the source code.

    // Push the function onto the stack to protect it from garbage collection
    push(OBJ_VAL(function));

    // Allocate and initialize the very first call frame for the top-level script
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    // Execute the byte-code within the virtual machine.
    return run();
}
