#ifndef PARSER_H
#define PARSER_H

#include "expression.h"
#include "object.h"
#include "statement.h"
#include "tokens.h"

typedef struct {
  Token_Vector *tok_list;
  int index;
} Parser;

void printAST(Expression *root, Arena *scratch);
// void printASTPost(Expression *root);

// Grammar implementation for statements
Statement *stmt(Parser *parser);
Statement *parse_var_stmt(Parser *parser);
Statement *parse_block_stmt(Parser *parser);
Statement *parse_if_stmt(Parser *parser);
Statement *parse_print_stmt(Parser *parser);
Statement *parse_expr_stmt(Parser *parser);

// Grammar implementation for expressions
Expression *expr(Parser *parser);
Expression *assignment(Parser *parser);
Expression *logic_or(Parser *parser);
Expression *logic_and(Parser *parser);
Expression *equality(Parser *parser);
Expression *comparison(Parser *parser);
Expression *term(Parser *parser);
Expression *factor(Parser *parser);
Expression *unary(Parser *parser);
Expression *primary(Parser *parser);
#endif