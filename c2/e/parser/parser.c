#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "utils.h"
#include "unicodeUtf8.h"
#include <string.h>
#include <ctype.h>

struct keywordToken
{
    char *keyword;
    uint8_t length;
    TokenType token;
};

struct keywordToken keywordsToken[] = {
    {"var", 3, TOKEN_VAR},
    {"fun", 3, TOKEN_FUN},
    {"if", 2, TOKEN_IF},
    {"else", 4, TOKEN_ELSE},
    {"true", 4, TOKEN_TRUE},
    {"false", 5, TOKEN_FALSE},
    {"while", 5, TOKEN_WHILE},
    {"for", 3, TOKEN_FOR},
    {"break", 5, TOKEN_BREAK},
    {"continue", 8, TOKEN_CONTINUE},
    {"return", 6, TOKEN_RETURN},
    {"null", 4, TOKEN_NULL},
    {"class", 5, TOKEN_CLASS},
    {"is", 2, TOKEN_IS},
    {"static", 6, TOKEN_STATIC},
    {"this", 4, TOKEN_THIS},
    {"super", 5, TOKEN_SUPER},
    {"import", 6, TOKEN_IMPORT},
    {NULL, 0, TOKEN_UNKNOWN}};

static TokenType idOrkeyword(const char *start, uint32_t length)
{
    uint32_t idx = 0;
    while (keywordsToken[idx].keyword != NULL)
    {
        if (keywordsToken[idx].length == length && memcmp(keywordsToken[idx].keyword, start, length) == 0)
        {
            return keywordsToken[idx].token;
        }
        idx++;
    }
    return TOKEN_ID;
}

char lookAheadChar(Parser *parser)
{
    return *parser->nextCharPtr;
}

static void getNextChar(Parser *parser)
{
    parser->curChar = *parser->nextCharPtr++;
}

static bool matchNextChar(Parser *parser, char expectedChar)
{
    if (lookAheadChar(parser) == expectedChar)
    {
        getNextChar(parser);
        return true;
    }
    return false;
}

static void skipBlanks(Parser *parser)
{
    while (isspace(parser->curChar))
    {
        if (parser->curChar == '\n')
        {
            parser->curToken.lineNo++;
        }
        getNextChar(parser);
    }
}