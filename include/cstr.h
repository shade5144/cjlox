#ifndef C_STR
#define C_STR

#include <sys/types.h>

typedef struct {
  char *data;
  size_t length;
} cstr;

#endif
