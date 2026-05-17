#ifndef EVAL_H
#define EVAL_H

#include "object.h"
#include "parser.h"
#include "statement.h"

Statement *evalStatement(Statement *stmt);
Object *evalExpression(Expression *exp);

#endif