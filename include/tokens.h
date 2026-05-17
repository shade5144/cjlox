#ifndef TOKEN_H
#define TOKEN_H

#include "arenalloc.h"
#include "cstr.h"
#include <sys/types.h>

typedef enum {
  TOK_AND = 0,
  TOK_CLASS,
  TOK_ELSE,
  TOK_FALSE,
  TOK_FOR,
  TOK_FUN,
  TOK_IF,
  TOK_NIL,
  TOK_OR,
  TOK_PRINT,
  TOK_QUIT,
  TOK_RETURN,
  TOK_SUPER,
  TOK_THIS,
  TOK_TRUE,
  TOK_VAR,
  TOK_WHILE,

  TOK_IDENTIFIER,
  TOK_STRING,
  TOK_NUMBER,

  TOK_LEFT_PAREN,
  TOK_RIGHT_PAREN,
  TOK_LEFT_BRACE,
  TOK_RIGHT_BRACE,
  TOK_COMMA,
  TOK_DOT,
  TOK_MINUS,
  TOK_PLUS,
  TOK_SEMICOLON,
  TOK_SLASH,
  TOK_STAR,

  // One or two character tokens.
  TOK_BANG,
  TOK_BANG_EQUAL,
  TOK_EQUAL,
  TOK_EQUAL_EQUAL,
  TOK_GREATER,
  TOK_GREATER_EQUAL,
  TOK_LESS,
  TOK_LESS_EQUAL,
  TOK_EOF,
  INVALID_TOKEN,
} Token_Type;

typedef struct {
  cstr tok_string;
  int line;
  int index;
  Token_Type tok_type;
} Token;

typedef struct {
  Token **tokens;
  int length;
  int capacity;
} Token_Vector;

char *formatTokenType(Token_Type type, Arena *scratch, size_t *ret_bytes,
                      char *token_names[40]);

#endif