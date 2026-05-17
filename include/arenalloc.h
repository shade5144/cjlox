#ifndef ARENALLOC_H
#define ARENALLOC_H

#include <stdint.h>
#include <sys/types.h>

typedef struct {
  char *a_buf;
  uintptr_t a_buflen;
  uintptr_t a_curoff;
  uintptr_t a_prevoff;
  void *_cur_page;
} Arena;

void arenaInit(Arena *arena, size_t reserve_gigs); // mallocs

// mmap memory for backing buffer
void *arenaAlloc(Arena *arena, size_t size, size_t align);
// Reset arena's pointers
void arenaFreeAll(Arena *arena);
// Call munmap on backing buffer
void arenaRelease(Arena *arena);

#endif
