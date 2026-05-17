#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "object.h"
#include "tokens.h"

typedef enum { EXP_LITERAL, EXP_BINARY, EXP_UNARY } Expression_Type;

typedef struct Expression {
  struct {
    int index : 16;
    Expression_Type type : 16;
  };

  int line;

  union {
    Token_Type op;
    Object *literal;
  };

  struct Expression *right;
  struct Expression *left;
} Expression;

#endif