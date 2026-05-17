#include "parser.h"
#include "arenalloc.h"
#include "expression.h"
#include "loxerror.h"
#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define form_binary(left, op, right) createExpression(left, right, op);
#define form_unary(left, op) createExpression(left, NULL, op);
#define form_literal(value) createExpression(NULL, NULL, value);

#define assign_tail(cur_tail, ret_stmt)                                        \
  if (cur_tail != NULL) {                                                      \
    cur_tail->next = ret_stmt;                                                 \
  }                                                                            \
  cur_tail = ret_stmt;                                                         \
  // TODO:
  // - No need to create an Object every single time for a nil value. Create a
  // single nil Object and point to it instead.
  // - Add TOK_STRING, TOK_IDENTIFIER, TOK_TRUE, TOK_FALSE, TOK_NIL support for
  // primary

extern Arena *g_exp_arena;
extern Arena *g_obj_arena;
extern Arena *g_stmt_arena;

static Statement *g_tail_stmt = NULL;

#define try_statement_form(statement_func, ret, parser)                        \
  ret = statement_func(parser);                                                \
  if (ret) {                                                                   \
    return ret;                                                                \
  }

void printASTPre(Expression *root, int nest_level, Arena *scratch,
                 char *token_names[40]) {
  for (int i = 0; i < nest_level * 2; i++) {
    printf(".");
  }

  size_t trim_ind;

  if (root->left == NULL) {
    printf("lit: ");
    char *ret = formatObject(root->literal, scratch, &trim_ind);
    ret[trim_ind] = '\0';

    printf("%s\n", ret);
    arenaFreeAll(scratch);
  } else if (root->right == NULL) {
    char *ret = formatTokenType(root->op, scratch, &trim_ind, token_names);
    ret[trim_ind] = '\0';

    printf("%s\n", ret);
    arenaFreeAll(scratch);
    printASTPre(root->left, nest_level + 1, scratch, token_names);
  } else {
    char *ret = formatTokenType(root->op, scratch, &trim_ind, token_names);
    ret[trim_ind] = '\0';

    printf("%s\n", ret);
    arenaFreeAll(scratch);
    printASTPre(root->left, nest_level + 1, scratch, token_names);
    printASTPre(root->right, nest_level + 1, scratch, token_names);
  }
}

void printAST(Expression *root, Arena *scratch) {
  char *token_names[40] = {
      "TOK_AND",         "TOK_CLASS",       "TOK_ELSE",
      "TOK_FALSE",       "TOK_FOR",         "TOK_FUN",
      "TOK_IF",          "TOK_NIL",         "TOK_OR",
      "TOK_PRINT",       "TOK_RETURN",      "TOK_SUPER",
      "TOK_THIS",        "TOK_TRUE",        "TOK_VAR",
      "TOK_WHILE",       "TOK_IDENTIFIER",  "TOK_STRING",
      "TOK_NUMBER",      "TOK_LEFT_PAREN",  "TOK_RIGHT_PAREN",
      "TOK_LEFT_BRACE",  "TOK_RIGHT_BRACE", "TOK_COMMA",
      "TOK_DOT",         "TOK_MINUS",       "TOK_PLUS",
      "TOK_SEMICOLON",   "TOK_SLASH",       "TOK_STAR",
      "TOK_BANG",        "TOK_BANG_EQUAL",  "TOK_EQUAL",
      "TOK_EQUAL_EQUAL", "TOK_GREATER",     "TOK_GREATER_EQUAL",
      "TOK_LESS",        "TOK_LESS_EQUAL",  "TOK_EOF",
      "INVALID_TOKEN"};

  printASTPre(root, 0, scratch, token_names);
}

// void printASTPost(Expression *root)
// {
// 	if(root->left == NULL)
// 	{
// 		printf("Leaf: %lf\n", root->literal);
// 	}
// 	else if(root->right == NULL)
// 	{
// 		printASTPost(root->left);
// 		printf("Unary op: %c\n", root->op);
// 	}
// 	else
// 	{
// 		printASTPost(root->left);
// 		printASTPost(root->right);
// 		printf("Binary op: %c\n", root->op);
// 	}
// }

Expression *createExpression(Expression *left, Expression *right, void *value) {
  Expression *new =
      (Expression *)arenaAlloc(g_exp_arena, sizeof(Expression), 8);

  new->left = left;
  new->right = right;

  if (left == NULL) {
    new->type = EXP_LITERAL;

    new->literal = value;
    return new;
  } else if (right == NULL) {
    new->type = EXP_UNARY;
  } else {
    new->type = EXP_BINARY;
  }

  Token *cur_tok = (Token *)(value);

  new->op = cur_tok->tok_type;
  new->index = cur_tok->index;
  new->line = cur_tok->line;

  // printf("--->\n");
  // printExpArena();
  // printf("<---\n");

  return new;
}

// Refactor this into ret1 || ret2 || ret3... format
Statement *stmt(Parser *parser) {
  Token **tok_list = parser->tok_list->tokens;
  Token *cur_tok = tok_list[parser->index];

  if (cur_tok->tok_type == TOK_EOF) {
    printf("Reached EOF, no more statements\n");
    return NULL;
  }

  Statement *ret;

  try_statement_form(parse_var_stmt, ret, parser);
  try_statement_form(parse_print_stmt, ret, parser);
  try_statement_form(parse_if_stmt, ret, parser);
  try_statement_form(parse_expr_stmt, ret, parser);

  // Semantics for block statements start here. Store a copy of the previous
  // tail statement
  Statement *prev_tail = g_tail_stmt;

  g_tail_stmt = NULL;

  ret = parse_block_stmt(parser);

  // Restore the value
  g_tail_stmt = prev_tail;

  if (ret) {
    return ret;
  }

  return NULL;
}

Statement *parse_var_stmt(Parser *parser) {
  Token **tok_list = parser->tok_list->tokens;
  Token *var_tok = tok_list[parser->index];

  if (var_tok->tok_type == TOK_VAR) {
    parser->index += 1;
    Token *id_tok = tok_list[parser->index];

    if (id_tok->tok_type == TOK_IDENTIFIER) {
      parser->index += 1;
      Token *assign_tok = tok_list[parser->index];

      if (assign_tok->tok_type == TOK_EQUAL) {
        parser->index += 1;
        Expression *var_expr = expr(parser);

        if (var_expr) {
          Token *semicolon_tok = tok_list[parser->index];

          if (semicolon_tok->tok_type == TOK_SEMICOLON) {
            parser->index += 1;
            Statement *ret =
                (Statement *)arenaAlloc(g_stmt_arena, sizeof(Statement), 8);

            ret->type = STMT_VAR;

            Var_Statement *var_stmt = (Var_Statement *)arenaAlloc(
                g_stmt_arena, sizeof(Var_Statement), 8);

            var_stmt->assignment = var_expr;
            var_stmt->identifier.data = id_tok->tok_string.data;
            var_stmt->identifier.length = id_tok->tok_string.length;

            ret->params = var_stmt;

            assign_tail(g_tail_stmt, ret);

            ret->next = NULL;
            return ret;
          }

          formatError("Expected semicolon after statement", semicolon_tok->line,
                      semicolon_tok->index);
          return NULL;
        }

        formatError("Expected expression after assignment operator",
                    assign_tok->line, assign_tok->index);
        return NULL;
      }

      formatError("Expected assignment after identifier in var declaration",
                  assign_tok->line, assign_tok->index);
      return NULL;
    }

    formatError("Expected identifier after var", id_tok->line, id_tok->index);
    return NULL;
  }

  return NULL;
}

Statement *parse_block_stmt(Parser *parser) {
  Token **tok_list = parser->tok_list->tokens;

  if (tok_list[parser->index]->tok_type == TOK_LEFT_BRACE) {
    Token *block_start = tok_list[parser->index];
    parser->index += 1;

    Statement *block_stmt = stmt(parser);
    int parser_ind = parser->index;
    Statement *temp;

    do {
      temp = stmt(parser);

      if (!temp) {
        parser->index = parser_ind;
      } else {
        parser_ind = parser->index;
      }
    } while (temp);

    if (block_stmt) {
      Token *block_end = tok_list[parser->index];

      if (block_end->tok_type == TOK_RIGHT_BRACE) {
        parser->index += 1;
        Statement *ret_block = arenaAlloc(g_stmt_arena, sizeof(Statement), 8);

        ret_block->type = STMT_BLOCK;
        ret_block->params = block_stmt;

        // Block statements defer to the next statement of the previous
        // statement
        ret_block->next = NULL;

        return ret_block;
      }

      char fmt[64];
      sprintf(fmt, "Expected '}' for block starting at %d:%d",
              block_start->line, block_start->index);

      formatError(fmt, block_end->line, block_end->index);
      return NULL;
    }

    formatError("Invalid statement for block", block_start->line,
                block_start->index);
    return NULL;
  }

  return NULL;
}

// Try flattening control flow for this. Wow
Statement *parse_if_stmt(Parser *parser) {
  Token **tok_list = parser->tok_list->tokens;

  if (tok_list[parser->index]->tok_type == TOK_IF) {
    parser->index += 1;
    Token *cond_start = tok_list[parser->index];

    if (tok_list[parser->index]->tok_type == TOK_LEFT_PAREN) {
      parser->index += 1;
      Token *expr_start_tok = tok_list[parser->index];

      Expression *cond_expr = expr(parser);

      if (cond_expr) {
        Token *is_right_paren = tok_list[parser->index];

        if (is_right_paren->tok_type == TOK_RIGHT_PAREN) {
          parser->index += 1;
          Token *body_start = tok_list[parser->index];
          Statement *body = stmt(parser);

          if (body) {
            Statement *ret_if = arenaAlloc(g_stmt_arena, sizeof(Statement), 8);

            ret_if->type = STMT_IF;
            If_Statement *if_struct =
                arenaAlloc(g_stmt_arena, sizeof(If_Statement), 8);

            if_struct->body = body;
            if_struct->cond = cond_expr;

            if (tok_list[parser->index]->tok_type == TOK_ELSE) {
              parser->index += 1;
              Statement *tail_before = g_tail_stmt;
              if_struct->else_stmt = stmt(parser);
              g_tail_stmt = tail_before;
            } else {
              if_struct->else_stmt = NULL;
            }

            ret_if->params = if_struct;

            assign_tail(g_tail_stmt, ret_if);

            ret_if->next = NULL;

            return ret_if;
          }

          formatError("Invalid body for if statement", body_start->line,
                      body_start->index);
          return NULL;
        }

        formatError(
            "Expected ')' after conditional expression for if statement",
            is_right_paren->line, is_right_paren->index);
        return NULL;
      }

      formatError("Bad expression for if condition", expr_start_tok->line,
                  expr_start_tok->index);
      return NULL;
    }

    formatError("Expected '(' after 'if' keyword", cond_start->line,
                cond_start->index);
    return NULL;
  }

  return NULL;
}

Statement *parse_print_stmt(Parser *parser) {
  Token **tok_list = parser->tok_list->tokens;

  if (tok_list[parser->index]->tok_type == TOK_PRINT) {
    parser->index += 1;

    Expression *ret_print = expr(parser);

    Token *cur_tok = tok_list[parser->index];

    if (cur_tok->tok_type == TOK_SEMICOLON) {
      parser->index += 1;

      Statement *ret = arenaAlloc(g_stmt_arena, sizeof(Statement), 8);

      ret->type = STMT_PRINT;
      ret->params = ret_print;

      assign_tail(g_tail_stmt, ret);

      ret->next = NULL;

      return ret;
    }

    formatError("Expected semicolon at the end of the line", cur_tok->line,
                cur_tok->index);
  }

  return NULL;
}

Statement *parse_expr_stmt(Parser *parser) {
  Token **tok_list = parser->tok_list->tokens;
  Token *cur_tok = tok_list[parser->index];

  int parser_prev_ind = parser->index;
  Expression *ret_expr = expr(parser);

  if (ret_expr != NULL) {
    if (tok_list[parser->index]->tok_type == TOK_SEMICOLON) {
      parser->index += 1;

      Statement *ret = arenaAlloc(g_stmt_arena, sizeof(Statement), 8);

      ret->type = STMT_EXPR;
      ret->params = ret_expr;

      assign_tail(g_tail_stmt, ret);

      ret->next = NULL;

      return ret;
    }

    formatError("Expected semicolon at the end of the line", cur_tok->line,
                cur_tok->index);
  }

  parser->index = parser_prev_ind;
  return NULL;
}

Expression *expr(Parser *parser) { return assignment(parser); }

Expression *assignment(Parser *parser) {
  Token **tok_list = parser->tok_list->tokens;
  Token *id_tok = tok_list[parser->index];

  int prev_index = parser->index;
  if (id_tok->tok_type == TOK_IDENTIFIER) {
    parser->index += 1;
    Token *assign = tok_list[parser->index];

    if (assign->tok_type == TOK_EQUAL) {
      parser->index += 1;
      Expression *nested_assign = assignment(parser);
      Object *id_obj = (Object *)arenaAlloc(g_obj_arena, sizeof(Object), 8);

      id_obj->type = OBJ_IDENTIFIER;
      id_obj->data = id_tok->tok_string.data;
      id_obj->size = id_tok->tok_string.length;

      Expression *identifier = form_literal(id_obj);

      return form_binary(identifier, assign, nested_assign);
    }
  }

  parser->index = prev_index;
  return logic_or(parser);
}

Expression *logic_or(Parser *parser) {
  Expression *init = logic_and(parser);
  Token *tok;

  if (init != NULL) {
    while ((tok = parser->tok_list->tokens[parser->index])->tok_type ==
           TOK_OR) {
      parser->index += 1;

      Expression *right = logic_and(parser);
      init = form_binary(init, tok, right);
    }
  }

  return init;
}

Expression *logic_and(Parser *parser) {
  Expression *init = equality(parser);
  Token *tok;

  if (init != NULL) {
    while ((tok = parser->tok_list->tokens[parser->index])->tok_type ==
           TOK_AND) {
      parser->index += 1;

      Expression *right = equality(parser);
      init = form_binary(init, tok, right);
    }
  }

  return init;
}

Expression *equality(Parser *parser) {
  Expression *init = comparison(parser);
  Token *tok;

  if (init != NULL) {
    while ((tok = parser->tok_list->tokens[parser->index])->tok_type ==
               TOK_BANG_EQUAL ||
           (tok->tok_type == TOK_EQUAL_EQUAL)) {
      parser->index += 1;

      Expression *right = comparison(parser);
      init = form_binary(init, tok, right);
    }
  }

  return init;
}

Expression *comparison(Parser *parser) {
  Expression *init = term(parser);
  Token *tok;

  if (init != NULL) {
    while ((tok = parser->tok_list->tokens[parser->index])->tok_type ==
               TOK_GREATER ||
           (tok->tok_type == TOK_GREATER_EQUAL) ||
           (tok->tok_type == TOK_LESS) || (tok->tok_type == TOK_LESS_EQUAL)) {
      parser->index += 1;

      Expression *right = term(parser);
      init = form_binary(init, tok, right);
    }
  }

  return init;
}

Expression *term(Parser *parser) {
  Expression *init = factor(parser);
  Token *tok;

  if (init != NULL) {
    while ((tok = parser->tok_list->tokens[parser->index])->tok_type ==
               TOK_PLUS ||
           (tok->tok_type == TOK_MINUS)) {
      parser->index += 1;

      Expression *right = factor(parser);
      init = form_binary(init, tok, right);
    }
  }

  return init;
}

Expression *factor(Parser *parser) {
  Expression *init = unary(parser);
  Token *tok;

  if (init != NULL) {
    while ((tok = parser->tok_list->tokens[parser->index])->tok_type ==
               TOK_STAR ||
           (tok->tok_type == TOK_SLASH)) {
      parser->index += 1;

      Expression *right = unary(parser);
      init = form_binary(init, tok, right);
    }
  }

  return init;
}

Expression *unary(Parser *parser) {
  Token *cur_tok = parser->tok_list->tokens[parser->index];

  if (cur_tok->tok_type == TOK_PLUS || (cur_tok->tok_type == TOK_MINUS)) {
    parser->index += 1;
    Expression *single = unary(parser);

    if (single != NULL) {
      return form_unary(single, cur_tok);
    }

    return NULL;
  } else {
    Expression *ret = primary(parser);
    parser->index += 1;
    return ret;
  }
}

Expression *primary(Parser *parser) {
  Token *cur_tok = parser->tok_list->tokens[parser->index];
  Object *buf;

  switch (cur_tok->tok_type) {
  case TOK_NUMBER: {
    buf = (Object *)arenaAlloc(g_obj_arena, sizeof(Object), 8);
    buf->data = arenaAlloc(g_obj_arena, sizeof(double), 8);
    buf->size = sizeof(double);
    buf->type = OBJ_NUMBER;

    if (!sscanf(cur_tok->tok_string.data, "%lf", (double *)buf->data)) {
      formatError("Couldn't parse number", cur_tok->line, cur_tok->index);
      return NULL;
    }

    return form_literal(buf);
  }
  case TOK_TRUE:
  case TOK_FALSE: {
    buf = (Object *)arenaAlloc(g_obj_arena, sizeof(Object), 8);
    buf->data = cur_tok->tok_type == TOK_TRUE
                    ? (void *)0x01
                    : (void *)0x00; // Encode booleans as invalid pointers
    buf->size = 1;
    buf->type = OBJ_BOOLEAN;

    return form_literal(buf);
  }
  case TOK_STRING: {
    buf = (Object *)arenaAlloc(g_obj_arena, sizeof(Object), 8);

    buf->size = cur_tok->tok_string.length;

    // String interning goes here
    buf->data = malloc(buf->size + 1);

    memcpy(buf->data, cur_tok->tok_string.data, buf->size);
    char *buf_string = (char *)(buf->data);
    buf_string[buf->size] = '\0';

    buf->type = OBJ_STRING;

    return form_literal(buf);
  }
  case TOK_NIL: {
    buf = (Object *)arenaAlloc(g_obj_arena, sizeof(Object), 8);
    buf->type = OBJ_NIL;

    return form_literal(buf);
  }
  case TOK_LEFT_PAREN: {
    parser->index += 1;
    Expression *ret = expr(parser);

    if (parser->tok_list->tokens[parser->index]->tok_type == TOK_RIGHT_PAREN) {
      return ret;
    }

    formatError("Matching closing parenthesis not found", cur_tok->line,
                cur_tok->index);
    return NULL;
  }
  case TOK_IDENTIFIER: {
    buf = (Object *)arenaAlloc(g_obj_arena, sizeof(Object), 8);

    buf->type = OBJ_IDENTIFIER;
    buf->data = cur_tok->tok_string.data;
    buf->size = cur_tok->tok_string.length;

    return form_literal(buf);
  }
  default: {
    return NULL; // Get rid of compiler warnings
  }
  }
}