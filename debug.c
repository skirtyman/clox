#include <stdio.h>

#include "debug.h"

void disassembleChunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);

    // Iterate over all of the positions within the chunks' code that is occupied by a byte-code instruction (== count).
    // The lack of iterator mean to loop until the condition is no longer satisfied.
    // We assign offset to disassembleInstruction as it will return the position of the next instruction, allowing for
    // variable-sized instructions.
    for (int offset = 0; offset < chunk -> count;)
        offset = disassembleInstruction(chunk, offset);
}

// Print an instruction that is 1 byte in size.
static int simpleInstruction(const char* name, int offset)
{
    printf("%s\n", name);
    // Return the new position of the instruction to be printed. This is 1 byte along == +1 to current offset as each
    // OP-CODE is currently 1 byte in size.
    return offset + 1;
}

int disassembleInstruction(Chunk* chunk, int offset)
{
    // Print the offset of within the given chunk.
    printf("%04d ", offset);

    // Extract the instruction within the chunk at the specified offset.
    uint8_t instruction = chunk -> code[offset];
    switch (instruction)
    {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
