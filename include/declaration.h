#ifndef DECLARATION_H
#define DECLARATION_H

typedef enum {
  DECL_VAR,
  DECL_STMT,
} Declaration_Type;

typedef struct {
  Declaration_Type type;
  void *params;
} Declaration;

#endif