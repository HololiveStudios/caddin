#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct Arena {
    uint8_t *buffer;
    size_t   capacity;
    size_t   offset;
};

Arena *arena_create(size_t capacity) {
    if (capacity == 0) return NULL;
    Arena *a = (Arena *)malloc(sizeof(Arena));
    if (!a) return NULL;

    a->buffer = (uint8_t *)malloc(capacity);
    if (!a->buffer) {
        free(a);
        return NULL;
    }
    a->capacity = capacity;
    a->offset = 0;
    return a;
}

void arena_destroy(Arena *arena) {
    if (!arena) return;
    if (arena->buffer) {
        free(arena->buffer);
        arena->buffer = NULL;
    }
    free(arena);
}

void *arena_alloc_aligned(Arena *arena, size_t bytes, size_t alignment) {
    if (!arena || !arena->buffer || bytes == 0) return NULL;
    if (alignment == 0) alignment = sizeof(void *);

    /* Alinhar offset atual */
    size_t current_addr = (size_t)(arena->buffer + arena->offset);
    size_t padding = 0;
    size_t modulo = current_addr % alignment;
    if (modulo != 0) {
        padding = alignment - modulo;
    }

    if (arena->offset + padding + bytes > arena->capacity) {
        /* Estouro de capacidade da arena pré-alocada */
        return NULL;
    }

    arena->offset += padding;
    void *ptr = (void *)(arena->buffer + arena->offset);
    arena->offset += bytes;
    return ptr;
}

void *arena_alloc(Arena *arena, size_t bytes) {
    return arena_alloc_aligned(arena, bytes, 16);
}

void *arena_alloc_zero(Arena *arena, size_t bytes) {
    void *ptr = arena_alloc(arena, bytes);
    if (ptr) {
        memset(ptr, 0, bytes);
    }
    return ptr;
}

void arena_reset(Arena *arena) {
    if (!arena) return;
    arena->offset = 0;
}

size_t arena_used(const Arena *arena) {
    return arena ? arena->offset : 0;
}

size_t arena_capacity(const Arena *arena) {
    return arena ? arena->capacity : 0;
}
