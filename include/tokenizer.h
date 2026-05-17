#ifndef TOKENIZER_H
#define TOKENIZER_H

#define NUM_KEYWORDS 17

#include "arenalloc.h"
#include "cstr.h"
#include "tokens.h"
#include <sys/types.h>

void pushTokenVector(Token_Vector *vec, Token *tok);

Token *createToken(Token_Type type, cstr *tok_literal, Arena *tok_arena);

Token_Type binarySearchKeyword(char *string, char *keywords[NUM_KEYWORDS]);
int tokenizeText(cstr *line, Token_Vector *tok_list, Arena *scratch_arena,
                 Arena *token_arena);

void printTokList(Token_Vector *tok_list, Arena *scratch);

#endif
