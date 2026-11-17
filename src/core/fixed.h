#ifndef PET_FIXED_H
#define PET_FIXED_H

#include <stdint.h>
#include <stdbool.h>

/* Ponto fixo Q8.8: 8 bits inteiros, 8 bits fracionários */
#define FIX_SHIFT 8
#define FIX_ONE   (1 << FIX_SHIFT)
#define FIX_HALF  (1 << (FIX_SHIFT - 1))

typedef int16_t fix16_t;
typedef int32_t fix32_t;

static inline fix16_t fix_from_float(float f) {
    float scaled = f * (float)FIX_ONE;
    if (scaled > 32767.0f) return 32767;
    if (scaled < -32768.0f) return -32768;
    return (fix16_t)(scaled >= 0.0f ? (scaled + 0.5f) : (scaled - 0.5f));
}

static inline float fix_to_float(fix16_t x) {
    return (float)x / (float)FIX_ONE;
}

static inline fix16_t fix_from_int(int x) {
    return (fix16_t)(x << FIX_SHIFT);
}

static inline int fix_to_int(fix16_t x) {
    return x >> FIX_SHIFT;
}

static inline fix16_t fix_mul(fix16_t a, fix16_t b) {
    fix32_t prod = (fix32_t)a * (fix32_t)b;
    return (fix16_t)((prod + FIX_HALF) >> FIX_SHIFT);
}

static inline fix16_t fix_div(fix16_t a, fix16_t b) {
    if (b == 0) return (a >= 0) ? 32767 : -32768;
    fix32_t temp = ((fix32_t)a << FIX_SHIFT);
    return (fix16_t)(temp / b);
}

static inline int16_t sat_add16(int16_t a, int16_t b) {
    int32_t res = (int32_t)a + (int32_t)b;
    if (res > 32767) return 32767;
    if (res < -32768) return -32768;
    return (int16_t)res;
}

static inline int16_t sat_sub16(int16_t a, int16_t b) {
    int32_t res = (int32_t)a - (int32_t)b;
    if (res > 32767) return 32767;
    if (res < -32768) return -32768;
    return (int16_t)res;
}

static inline int8_t sat_add8(int8_t a, int8_t b) {
    int16_t res = (int16_t)a + (int16_t)b;
    if (res > 127) return 127;
    if (res < -128) return -128;
    return (int8_t)res;
}

static inline int8_t sat_clamp8(int32_t val) {
    if (val > 127) return 127;
    if (val < -128) return -128;
    return (int8_t)val;
}

static inline int16_t sat_clamp16(int32_t val) {
    if (val > 32767) return 32767;
    if (val < -32768) return -32768;
    return (int16_t)val;
}

/* Decaimento exponencial rápido aproximado por shift: v -= v >> shift */
static inline int16_t fix_decay_shift(int16_t v, int shift) {
    return (int16_t)(v - (v >> shift));
}

#endif /* PET_FIXED_H */
