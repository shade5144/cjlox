#include "arenalloc.h"
#include "loxerror.h"
#include "object.h"
#include "tokens.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

char *formatTokenType(Token_Type type, Arena *scratch, size_t *ret_bytes,
                      char *token_names[40]) {
  char *tok_string = token_names[(int)(type)];
  size_t len_string = strlen(tok_string) + 1;

  char *ret_scratch = arenaAlloc(scratch, len_string, 8);

  *ret_bytes = len_string;

  memcpy(ret_scratch, tok_string, len_string);

  return ret_scratch;
}

// Might be useful for the 'print' routine
char *formatObject(Object *obj, Arena *scratch, size_t *ret_bytes) {
  char buf[4096];
  size_t written = 0;

  if (obj == NULL) {
    return NULL;
  }

  switch (obj->type) {
  case OBJ_NUMBER: {
    written = sprintf(buf, "%lf", *((double *)(obj->data)));
    break;
  }

  case OBJ_STRING: {
    written = sprintf(buf, "%s", (char *)(obj->data));
    break;
  }

  case OBJ_BOOLEAN: {
    long value = (long)(obj->data);

    written = sprintf(buf, "%s", value == 1 ? "True" : "False");
    break;
  }

  case OBJ_NIL: {
    written = sprintf(buf, "nil");
    break;
  }
  default: {
    return NULL;
  }
  }

  *ret_bytes = written;
  char *ret_scratch = (char *)arenaAlloc(scratch, written, 8);

  memcpy(ret_scratch, buf, written);

  return ret_scratch;
}

void formatError(char *message, int line, int index) {
  printf("\033[1;34mFile:%d:%d: \033[1;31mError: \033[0m %s\n", line, index,
         message);
}