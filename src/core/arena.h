#ifndef PET_ARENA_H
#define PET_ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Arena Arena;

Arena *arena_create(size_t capacity);
void   arena_destroy(Arena *arena);
void  *arena_alloc(Arena *arena, size_t bytes);
void  *arena_alloc_aligned(Arena *arena, size_t bytes, size_t alignment);
void  *arena_alloc_zero(Arena *arena, size_t bytes);
void   arena_reset(Arena *arena);
size_t arena_used(const Arena *arena);
size_t arena_capacity(const Arena *arena);

#endif /* PET_ARENA_H */
