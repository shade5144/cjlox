#ifndef OBJECT_H
#define OBJECT_H

#include "arenalloc.h"
#include <sys/types.h>

typedef enum {
  OBJ_NIL = 0,
  OBJ_NUMBER,
  OBJ_STRING,
  OBJ_BOOLEAN,
  OBJ_IDENTIFIER,
} Object_Type;

typedef struct {
  Object_Type type;
  void *data;
  int size;
} Object;

char *formatObject(Object *obj, Arena *scratch, size_t *ret_bytes);
#endif