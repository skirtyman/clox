#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT, // Form >>> OP_CONSTANT <index> => VM must fetch the constant at <index> within the chunk's constant pool. This is a 2 byte instruction as index is 1-byte in size.
    OP_NIL, // Form >>> OP_NIL => Push a Nil value onto the VM stack. This is a 1-byte instruction.
    OP_TRUE, // Form >>> OP_TRUE => Push the boolean value `true` on to the VM stack. This is a 1-byte instruction.
    OP_FALSE, // Form >>> OP_FALSE => Push the boolean value `false` on to the VM stack. This is a 1-byte instruction.
    OP_POP, // Form >>> OP_POP => Pop a value from the top of the stack. This is a 1-byte instruction.
    OP_GET_LOCAL, // Form >>> OP_GET_LOCAL <index> => Fetches a local variable from the VM stack at the specified <index> slot and pushes it to the top of the stack. This is a 2-byte instruction.
    OP_SET_LOCAL, // Form >>> OP_SET_LOCAL <index> => Updates the local variable value directly in the VM stack at the specified <index> slot with the value from the top of the stack.
                  // This is a 2-byte instruction.
    OP_GET_GLOBAL, // Form >>> OP_GET_GLOBAL => Get a global variable with a specified index into the constant pool. This index points to the heap-allocated hash table.
    OP_DEFINE_GLOBAL, // Form >>> OP_DEFINE_GLOBAL => Define a global variable on the VM heap.
    OP_SET_GLOBAL, // Form >>> OP_SET_GLOBAL <index> => Sets a global variable, whose name is constantsTable[index] within the VM globals hash table to the item popped off of the VM stack.
    OP_GET_UPVALUE, // Form >>> OP_GET_UPVALUE <index> => Get the value of a local variable from a surrounding function scope using the specified upvalue index.
    OP_SET_UPVALUE, // Form >>> OP_SET_UPVALUE <index> => Sets the value of a local variable in a surrounding function scope using the specified upvalue index.
    OP_EQUAL, // Form >>> OP_EQUAL => Push the boolean value `a == b` on to the VM stack where a and b are popped operands. This is a 1-byte instruction.
    OP_GREATER, // Form >>> OP_GREATER => Push the boolean value `a > b` on to the VM stack where a and b are popped operands. This is a 1-byte instruction.
    OP_LESS, // Form >>> OP_LESS => Push the boolean value `a < b` on to the VM stack where a and b are popped operands. This is a 1-byte instruction.
    OP_ADD, // Form >>> OP_ADD => Returns the result of arithmetic addition of the 2 operands. This is a 1 byte instruction as OP_ADD, uses the operands stored in the stack and not
            //                    for the instruction itself. The same applies to OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE.
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT, // Form >>> OP_NOT => Return the logical NOT of the operand. This is 1 byte instruction.
    OP_NEGATE, // Form >>> OP_NEGATE <operand> => Returns negation of operand => OP_NEGATE 1.0 == -1.0. This is a 2 byte instruction.
    OP_PRINT, // Form >>> OP_PRINT => Signals the VM to print the value at the top of the stack. This is a 1-byte instruction.
    OP_JUMP, // Form >>> OP_JUMP <offset> => Unconditionally advances the instruction pointer forward by the 2-byte <offset>. This is a 3-byte instruction.
    OP_JUMP_IF_FALSE, // Form >>> OP_JUMP_IF_FALSE <offset> => Pops the condition value. If false, advances the instruction pointer by the 2-byte <offset>. This is a 3-byte instruction.
    OP_LOOP, // Form >>> OP_LOOP => Enables looping applying a negative jump to the `ip`.
    OP_CALL, // Form >>> OP_CALL <argCount> => Invokes a callable object at the stack slot below the <argCount> arguments, creating a new CallFrame. This is a 2-byte instruction.
    OP_CLOSURE, // Form >>> OP_CLOSURE <index> => Creates a closure over a given function whose name is specified within the constant pool at index [index].
    OP_CLOSE_UPVALUE, // Form >>> OP_CLOSE_UPVALUE => Hoists a local variable from the stack to the heap when its declaring scope exits, closing any open upvalues. This is a 1-byte instruction.
    OP_RETURN, // The VM has reached the end of a chunk of byte-code and returns the current execution frame (function).
} OpCode;

typedef struct
{
    int count; // Number of values within the constant pool.
    int capacity;
    uint8_t* code;
    int* lines; // Store a list of line numbers where each number is the line that a byte-code instruction belongs to.
    ValueArray constants; // Each chunk of byte-code contains an associated constant pool, allowing each chunk to have its own set of constants /
                          // not be limited by the size of the array.
} Chunk; // Represents a given chunk of byte-code as a dynamically sized array of bytes (the op-codes).

// Function to initialise a dynamic array storing a chunk of byte-code. We use a pointer to modify the chunk directly (pass-by-reference).
void initChunk(Chunk* chunk);

// This function resets a given chunk, freeing the memory allocated to its fields.
void freeChunk(Chunk* chunk);

// Function to write a byte-code instruction to the dynamic array including the line number it belongs to. Ensuring we resize where appropriate.
void writeChunk(Chunk* chunk, uint8_t byte, int line);

// Add a constant to the constant pool of the chunk.
int addConstant(Chunk* chunk, Value value);

#endif
