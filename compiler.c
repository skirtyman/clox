#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source)
{
    initScanner(source);

    int line = -1;
    for (;;)
    {
        Token token = scanToken();

        // Print the source line number only when transitioning to a new line.
        if (token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        }
        else
        {
            // The scanned token resides on the same line as the previous token.
            printf("   | ");
        }
         // Display the token's type enum value (the position within the enum), and its textual representation.
        printf("%2d '%.*s'\n", token.type, token.length, token.start);
        // Break out of the compilation loop upon encountering the End-Of-File token.
        if (token.type == TOKEN_EOF) break;
    }

}
