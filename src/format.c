#include "loxerror.h"
#include "object.h"
#include "tokens.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

void formatTokenType(Token_Type type, char *token_names[40]) {
  char *tok_string = token_names[(int)(type)];

  fprintf(stdout, "%s", tok_string);
}

// Might be useful for the 'print' routine
void formatObject(Object *obj) {
  switch (obj->type) {
  case OBJ_NUMBER: {
    fprintf(stdout, "%lf", *((double *)(obj->data)));
    break;
  }

  case OBJ_STRING: {
    fprintf(stdout, "%s", (char *)(obj->data));
    break;
  }

  case OBJ_BOOLEAN: {
    long value = (long)(obj->data);

    fprintf(stdout, "%s", value == 1 ? "True" : "False");
    break;
  }

  case OBJ_NIL: {
    fprintf(stdout, "nil");
    break;
  }
  default: {
    fprintf(stdout, "(NULL)");
  }
  }
}

void formatError(char *message, int line, int index) {
  printf("\033[1;34mFile:%d:%d: \033[1;31mError: \033[0m %s\n", line, index,
         message);
}