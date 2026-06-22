#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct
{
    const char* start; // The start of the current token being scanned.
    const char* current; // The current character being evaluated within the source code. Forming a sliding window approach.
    int line; // The current line within the Lox source code being scanned.
} Scanner;

// Define a scanner to be used.
Scanner scanner;

void initScanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

Token scanToken()
{
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    return errorToken("Unexpected character");
}
