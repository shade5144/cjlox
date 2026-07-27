#include "eval.h"
#include "expression.h"
#include "hashtable.h"
#include "loxerror.h"
#include "object.h"
#include "parser.h"
#include "tokens.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// TODO:
// - Implement logical operands for OBJ_STRING
// - Have to compare right operand with 0 for division to catch division by zero
// errors. But double comparison needs
//   to be done better

extern Arena *g_scratch_arena;
extern Environment *g_environment;

static Expression *g_cur_node;
static Statement *g_stmt_prev = NULL;

bool getNumberOrBoolean(Object *obj, double *ret) {
  if (obj->type == OBJ_NUMBER) {
    *ret = *(double *)(obj->data);
    return true;
  }

  if (obj->type == OBJ_BOOLEAN) {
    *ret = (double)((long)(obj->data));
    return true;
  }

  return false;
}

Statement *evalStatement(Statement *stmt) {
  Statement *ret = stmt->next;

  switch (stmt->type) {
  case STMT_EXPR: {
    Expression *to_eval = (Expression *)stmt->params;

    // Only pertinent for assignment statements and similar
    evalExpression(to_eval);
    break;
  }
  case STMT_PRINT: {
    printf("Printing expression..\n");
    Expression *to_print = (Expression *)stmt->params;
    Object *print_obj = evalExpression(to_print);

    if (print_obj->type == OBJ_IDENTIFIER) {
      cstr query_buf;
      query_buf.data = (char *)print_obj->data;
      query_buf.length = print_obj->size;

      Hash_Entry *entry = (Hash_Entry *)lookupFromTable(
          &(g_environment->tables[g_environment->cur_scope]), &query_buf);
      print_obj = entry->val;
    }

    formatObject(print_obj);
    printf("\n");
    break;
  }
  case STMT_BLOCK: {
    Statement *block_stmt = (Statement *)(stmt->params);

    if (g_stmt_prev) {
      ret = g_stmt_prev->next;
    }

    g_environment->cur_scope += 1;
    addScopeToEvmt(g_environment);
    Statement *temp = evalStatement(block_stmt);

    while (temp) {
      temp = evalStatement(temp);
    }

    removeScopeFromEvmt(g_environment);
    g_environment->cur_scope -= 1;
    break;
  }
  case STMT_IF: {
    If_Statement *if_stmt = (If_Statement *)(stmt->params);

    Object *cond_expr_ret = evalExpression(if_stmt->cond);
    int is_cond = 0;

    switch (cond_expr_ret->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double comp;

      getNumberOrBoolean(cond_expr_ret, &comp);
      is_cond = (int)comp;
      break;
    }
    case OBJ_STRING: {
      is_cond = cond_expr_ret->size;
      break;
    }
    default: {
      formatError("if condition evaluates to nil type\n", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }

    if (is_cond) {
      evalStatement(if_stmt->body);
    } else if (if_stmt->else_stmt) {
      evalStatement(if_stmt->else_stmt);
      return stmt->next; // Don't update g_stmt_prev
    }

    break;
  }
  case STMT_VAR: {
    Var_Statement *v_stmt = (Var_Statement *)(stmt->params);

    Object *val = evalExpression(v_stmt->assignment);

    if (!addObjectInScope(g_environment, &v_stmt->identifier, val)) {
      char buf[64];
      // Would be nice to have line information for initial declaration
      sprintf(buf, "Redeclaration of variable %s", v_stmt->identifier.data);
      formatError(buf, v_stmt->assignment->line, v_stmt->assignment->index);
      // Would be nice to have line information AT ALL. Maybe add to Statement
    }

    break;
  }
  default: {
    break;
  }
  }

  g_stmt_prev = stmt;
  return ret;
}

// These aren't the most obvious so will deal with them later
Object *evalLogicalAnd(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for logical and should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l && r;

      return left;
    }
    default: {
      formatError("Logical and not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalLogicalOr(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for logical or should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l || r;

      return left;
    }
    default: {
      formatError("Logical or not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalNotEqual(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError("The right operand for != should be a Number or a Boolean",
                    g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l != r;

      return left;
    }
    case OBJ_STRING: {
      // For now, leave this
    }
    default: {
      formatError("!= not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalEqual(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError("The right operand for == should be a Number or a Boolean",
                    g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l == r;

      return left;
    }
    case OBJ_STRING: {
      // For now, leave this
    }
    default: {
      formatError("== not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalGreaterThan(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for not > should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l > r;

      return left;
    }
    case OBJ_STRING: {
      // For now, leave this
    }
    default: {
      formatError("> not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalGreaterThanOrEqual(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for not >= should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l >= r;

      return left;
    }
    case OBJ_STRING: {
      // For now, leave this
    }
    default: {
      formatError(">= not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalLesserThan(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for not < should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l < r;

      return left;
    }
    case OBJ_STRING: {
      // For now, leave this
    }
    default: {
      formatError("< not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalLesserThanOrEqual(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for not <= should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l <= r;

      return left;
    }
    case OBJ_STRING: {
      // For now, leave this
    }
    default: {
      formatError("<= not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalPlus(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_NUMBER:
    case OBJ_BOOLEAN: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for addition should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l + r;

      return left;
    }
    case OBJ_STRING: {
      // Only supports concatenation between 2 strings
      if (right->type != OBJ_STRING) {
        formatError("Right value is not a string and can't be concatenated",
                    g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      int new_len = left->size + right->size;

      if (right->size > 0) {
        // Add string interning here
        char *new_string = malloc(new_len + 1);
        char *r_string = (char *)(right->data);
        char *l_string = (char *)(left->data);

        for (int i = 0; i < left->size; i++) {
          new_string[i] = l_string[i];
        }

        free(left->data);

        for (int i = 0; i < right->size; i++) {
          new_string[left->size + i] = r_string[i];
        }

        new_string[new_len] = '\0';

        left->data = new_string;
        left->size = new_len;
      }

      return left;
    }
    default: // Should handle nil here. For now booleans too
    {
      formatError("Addition not supported for nil type", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalMinus(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_BOOLEAN:
    case OBJ_NUMBER: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for addition should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l - r;

      return left;
    }
    case OBJ_STRING: {
      formatError("Subtraction not supported for strings", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    default: // Should handle nil and string here. Boolean is here for now too
    {
      formatError("Subtraction not supported for nil values", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalMultiply(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_BOOLEAN:
    case OBJ_NUMBER: {
      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for addition should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double l;
      getNumberOrBoolean(left, &l);

      double *ret_val = (double *)left->data;

      *ret_val = l * r;

      return left;
    }
    case OBJ_STRING: {
      formatError("Multiplication not supported for string", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    default: // Should handle nil and string here. Boolean is here for now too
    {
      formatError("Multiplication not supported for nil value",
                  g_cur_node->line, g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalDivide(Object *left, Object *right) {
  if (left && right) {
    switch (left->type) {
    case OBJ_BOOLEAN:
    case OBJ_NUMBER: {
      double l;
      getNumberOrBoolean(left, &l);

      double r;
      if (!getNumberOrBoolean(right, &r)) {
        formatError(
            "The right operand for addition should be a Number or a Boolean",
            g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      if (r == 0) {
        formatError("Division by zero", g_cur_node->line, g_cur_node->index);
        return NULL;
      }

      double *ret_val = (double *)left->data;

      *ret_val = l / r;

      return left;
    }
    case OBJ_STRING: {
      formatError("Division not supported for strings", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }

    default: // Should handle nil and string here. Boolean is here for now too
    {
      formatError("Division not supported for nil value", g_cur_node->line,
                  g_cur_node->index);
      return NULL;
    }
    }
  }

  return NULL;
}

Object *evalAssignment(cstr *id, Object *right) {
  Hash_Entry *entry = (Hash_Entry *)lookupFromTable(
      &(g_environment->tables[g_environment->cur_scope]), id);

  if (!entry) {
    char buf[64];
    sprintf(buf, "Variable %s not defined in current scope", id->data);
    formatError(buf, g_cur_node->line, g_cur_node->index);
    return NULL;
  }

  entry->val = right;

  return right;
}

Object *evalExpression(Expression *exp) {
  if (exp->type == EXP_LITERAL) {
    return exp->literal;
  }

  switch (exp->op) {
  // Assignment operator
  case TOK_EQUAL: {
    cstr l_val;
    l_val.data = (char *)exp->left->literal->data;
    l_val.length = exp->left->literal->size;
    Object *r_val = evalExpression(exp->right);

    g_cur_node = exp;

    return evalAssignment(&l_val, r_val);
  }

  // Logical Operators
  case TOK_AND: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalLogicalAnd(left, right);
  }
  case TOK_OR: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalLogicalOr(left, right);
  }
  // Equality Operators
  case TOK_BANG_EQUAL: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalNotEqual(left, right);
  }
  case TOK_EQUAL_EQUAL: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalEqual(left, right);
  }
  // Comparison Operators
  case TOK_GREATER: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalGreaterThan(left, right);
  }
  case TOK_GREATER_EQUAL: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalGreaterThanOrEqual(left, right);
  }
  case TOK_LESS: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalLesserThan(left, right);
  }
  case TOK_LESS_EQUAL: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalLesserThanOrEqual(left, right);
  }
  // Basic Arithmetic Operators
  case TOK_PLUS: {
    if (exp->type == EXP_UNARY) {
      return evalExpression(exp->left);
    }

    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;

    return evalPlus(left, right);
  }
  case TOK_MINUS: {
    if (exp->type == EXP_UNARY) {
      Object *single = evalExpression(exp->left);

      switch (single->type) {
      case OBJ_NUMBER: {
        double *ret = (double *)(single->data);
        *ret = -(*ret);

        return single;
      }
      case OBJ_BOOLEAN: {
        double *ret = (double *)(single->data);
        double val = (double)(*(char *)(single->data));
        *ret = -val;

        return single;
      }
      case OBJ_STRING: {
        formatError("Can't negate a string object", g_cur_node->line,
                    g_cur_node->index);
        return NULL;
      }
      default: // Only nil
      {
        formatError("Can't negate a nil object", g_cur_node->line,
                    g_cur_node->index);
        return NULL;
      }
      }
    }
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;
    return evalMinus(left, right);
  }
  case TOK_STAR: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;
    return evalMultiply(left, right);
  }
  case TOK_SLASH: {
    Object *left = evalExpression(exp->left);
    Object *right = evalExpression(exp->right);

    g_cur_node = exp;
    return evalDivide(left, right);
  }
  default: {
    return NULL; // Get rid of compiler warnings
  }
  }

  // Control never reaches here
  return 0;
}