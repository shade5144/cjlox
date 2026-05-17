#ifndef STATEMENT_H
#define STATEMENT_H

#include "expression.h"
#include "tokens.h"

typedef enum {
  STMT_VAR,
  STMT_EXPR,
  STMT_PRINT,
  STMT_BLOCK,
  STMT_IF,

} Statement_Type;

typedef struct Statement {
  Statement_Type type;
  void *params;
  struct Statement *next;
} Statement;

typedef struct {
  Expression *cond;
  Statement *body;
  Statement *else_stmt;
} If_Statement;

typedef struct {
  cstr identifier;
  Expression *assignment;
} Var_Statement;

#endif