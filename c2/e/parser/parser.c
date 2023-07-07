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

static void parseId(Parser *parser, TokenType *type)
{
    while (
        isalnum(parser->curChar) || parser->curChar == '_')
    {
        getNextChar(parser);
    }
    uint32_t length = (uint32_t)(parser->nextCharPtr - parser->curToken.start - 1);
    if (type != TOKEN_UNKNOWN)
    {
        parser->curToken.type = type;
    }
    else
    {
        parser->curToken.type = idOrkeyword(parser->curToken.start, length);
    }
    parser->curToken.length = length;
}

static void parseUnicodeCodePoint(Parser *parser, ByteBuffer *buf)
{
    uint32_t idx = 0;
    int value = 0;
    uint8_t digit = 0;
    while (idx++ < 4)
    {
        getNextChar(parser);
        if (parser->curChar == '\0')
        {
            LEX_ERROR(parser, "unterminated unicode!");
        }
        if (parser->curChar >= '0' && parser->curChar <= '9')
        {
            digit = parser->curChar - '0';
        }
        else if (parser->curChar >= 'a' && parser->curChar <= 'f')
        {
            digit = parser->curChar - 'a' + 10;
        }
        else if (parser->curChar >= 'A' && parser->curChar <= 'F')
        {
            digit = parser->curChar - 'A' + 10;
        }
        else
        {
            LEX_ERROR(parser, "invalid unicode!");
        }
        value = value * 16 | digit;
    }
    uint32_t byteNum = getByteNumOfEncodeUtf8(value);
    ASSERT(byteNum != 0, "utf8 encode bytes should between 1 and 4!");

    ByteBufferFillWrite(parser->vm, buf, 0, byteNum);
    encodeUtf8(buf->datas + buf->count - byteNum, value);
}

static void parseString(Parser *parser)
{
    ByteBuffer str;
    ByteBufferInit(&str);
    while (true)
    {
        getNextChar(parser);
        if (parser->curChar == '\0')
        {
            LEX_ERROR(parser, "unterminated string!");
        }

        if (parser->curChar == '"')
        {
            parser->curToken.type = TOKEN_STRING;
        }
        if (parser->curChar == '%')
        {
            if (!matchNextChar(parser, '('))
            {
                LEX_ERROR(parser, "'%' should followed by '('!");
            }
            if (parser->interpolationExpectRightParenNum > 0)
            {
                COMPILE_ERROR(parser, "sorry , i don't suppoert nest interpolate expression!");
            }
            parser->interpolationExpectRightParenNum = 1;
            parser->curToken.type = TOKEN_INTERPOLATION;
            break;
        }
        if (parser->curChar == '\\')
        {
            getNextChar(parser);
            switch (parser->curChar)
            {
            case '0':
                ByteBufferAdd(parser->vm, &str, '\0');
                break;
            case 'a':
                ByteBufferAdd(parser->vm, &str, '\a');
                break;
            case 'b':
                ByteBufferAdd(parser->vm, &str, '\b');
                break;
            case 'f':
                ByteBufferAdd(parser->vm, &str, '\f');
                break;
            case 'n':
                ByteBufferAdd(parser->vm, &str, '\n');
                break;
            case 'r':
                ByteBufferAdd(parser->vm, &str, '\r');
                break;
            case 't':
                ByteBufferAdd(parser->vm, &str, '\t');
                break;
            case 'u':
                parseUnicodeCodePoint(parser, &str);
                break;
            case '"':
                ByteBufferAdd(parser->vm, &str, '"');
                break;
            case '\\':
                ByteBufferAdd(parser->vm, &str, '\\');
                break;
            default:
                LEX_ERROR(parser, "unsupported escape \\%c", parser->curChar);
                break;
            }
        }
        else
        {
            ByteBufferAdd(parser->vm, &str, parser->curChar);
        }
    }
    ByteBufferClear(parser->vm, &str);
}