#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[])
{
    // Create a VM to be used within the interpreter's main entry point.
    initVM();

    Chunk chunk;
    initChunk(&chunk);

    // Testing adding a constant to the chunk's constant pool. Using OP_CONSTANT.
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    constant = addConstant(&chunk, 3.4);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_ADD, 123);

    constant = addConstant(&chunk, 5.6);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);

    writeChunk(&chunk, OP_DIVIDE, 123);
    writeChunk(&chunk, OP_NEGATE, 123);

    writeChunk(&chunk, OP_RETURN, 123);

    // Disassemble the created chunk to be outputted in human-readable format.
    disassembleChunk(&chunk, "test chunk");
    // Command the VM to interpret (run) a chunk of byte-code.
    interpret(&chunk);

    // The virtual machine is no longer in use and can therefore be freed.
    freeVM();
    freeChunk(&chunk);
    return 0;
}
