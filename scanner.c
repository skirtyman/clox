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

static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

// Advance the scanner to the next character to be scanned and return the scanned character.
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

// Return the current character to be scanned by the scanner.
static char peek()
{
    return *scanner.current;
}

static char peekNext()
{
    if (isAtEnd()) return '\0';
    // Look ahead to the character immediately following the current position without advancing the scanner.
    return scanner.current[1];
}

// Advance the scanner if the character to be scanned is what we expect, returning true if it is.
// This implements 1-stage lookahead.
static bool match(char expected)
{
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)(strlen(message));
    token.line = scanner.line;
    return token;
}

static void skipWhitespace()
{
    // Loop over all whitespace characters, skipping them until the character to be scanned is non-white-space and hence a token can be parsed.
    for (;;)
    {
        char c = peek();
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/': // Comments can also be treated in the same way as white-space and hence skipped.
                if (peekNext() == '/')
                {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd()) advance();
                }
                else
                {
                    // Stop skipping if the slash stands alone as a division operator token.
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type)
{
    // Verify that the total length of the scanned identifier matches the target keyword length,
    // and perform a memory comparison to ensure the remaining characters match exactly.
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType()
{
    // We can use a trie to identify the keywords within Lox, avoiding having to implement a hash table which is overkill.
    // This uses a switch case to implement the trie. See 16.4 for a conceptual explanation.
    switch (scanner.start[0])
    {
        // If you can a certain starting letter, scan a known number of characters ahead and check if that is equal to the known
        // rest of the keyword. If the keyword is matched then return its type otherwise, assume it is an identifier.
        // As defined by the Lox's keyword trie, we must also check beyond the first character for keywords on certain characters.
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1)
            {
                switch (scanner.start[1])
                {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1)
            {
                switch(scanner.start[1])
                {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }


    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    // Consume alphanumeric characters after the first letter to form the identifier / keyword name. Make a token using this name.
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

static Token number()
{
    // Advance the scanner if the character to be scanned is a digit and hence, the number being scanned continues.
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext()))
    {
        // Consume the '.'.
        advance();

        while(isDigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}

// Scan to produce a string token.
static Token string()
{
    // Consume characters until the next character is a closing quote and we are not at the end of the file.
    while (peek() != '"' && !isAtEnd())
    {
        // Lox supports multi-line strings so we must ensure that line numbers are correctly observed.
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (isAtEnd()) return errorToken("Unterminated string");

    // The closing quote.
    advance();
    return makeToken(TOKEN_STRING);
}

Token scanToken()
{
    // When starting the process of scanning, remove the leading white-space to ensure the next character is meaningful.
    // Reminder, we are scanning on demand of the compiler, meaning that we only trim white-space when the compiler needs a token.
    skipWhitespace();

    // This function is always called at the beginning of a new token => Start the scanner at the current token.
    scanner.start = scanner.current;

    // TOKEN_EOF => Signal to the compiler that scanning has been complete.
    if (isAtEnd()) return makeToken(TOKEN_EOF);

    // Scan tokens.
    char c = advance();

    // Scan numeric sub-strings and potential identifiers.
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch(c)
    {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '*': return makeToken(TOKEN_STAR);
        case '/': return makeToken(TOKEN_SLASH);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
    }

    return errorToken("Unexpected character");
}
