#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[])
{
    // Minor Change
    Chunk chunk;
    initChunk(&chunk);


    int constant1 = addConstant(&chunk, 2.4);
    writeChunk(&chunk, OP_CONSTANT, 122);
    writeChunk(&chunk, constant1, 122);

    // Testing adding a constant to the chunk's constant pool. Using OP_CONSTANT.
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_RETURN, 123);

    // Disassemble the created chunk to be outputted in human-readable format.
    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);
    return 0;
}
