#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[])
{
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN);

    // Disassemble the created chunk to be outputted in human-readable format.
    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);
    return 0;
}
