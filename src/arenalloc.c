#include "arenalloc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define ONE_MB 1024 * 1024

void arenaInit(Arena *arena, size_t reserve_mega) {
  int fd = open("/dev/zero", O_RDWR, 0777);

  size_t tentative_size = reserve_mega * ONE_MB;

  arena->a_buf = malloc(tentative_size);

  if (arena->a_buf == (void *)(-1)) {
    perror("malloc");
    exit(0);
  }

  close(fd);

  arena->a_buflen = tentative_size;
  arena->a_curoff = 0;
  arena->a_prevoff = 0;
  arena->_cur_page = NULL;
}

void *arenaAlloc(Arena *arena, size_t size, size_t align) {
  if (size == 0) {
    // "arenaAlloc: Request of size zero\n"
    return NULL;
  }

  if ((align & (align - 1)) != 0) {
    // "arenaAlloc: The alignment must be a power of 2\n"
    return NULL;
  }

  uintptr_t padding = (uintptr_t)(arena->a_buf + arena->a_curoff) & (align - 1);

  padding = (8 - padding) * (padding != 0);

  uintptr_t new_offset = size + padding;

  if (new_offset + arena->a_curoff > arena->a_buflen) {
    // "arenaAlloc: Exceeded memory limit\n"
    return NULL;
  }

  arena->a_prevoff = arena->a_curoff + padding;

  arena->a_curoff += new_offset;

  return arena->a_buf + arena->a_prevoff;
}

void arenaFreeAll(Arena *arena) {
  arena->a_curoff = 0;
  arena->a_prevoff = 0;
}

void arenaRelease(Arena *arena) { free(arena->a_buf); }
