#ifndef HYBRID_PRNG_H
#define HYBRID_PRNG_H

#include <stdint.h>

/*
 * hybrid_prng.h - Generador PRNG híbrido moderno (C11)
 *
 * Combina middle-square de von Neumann (componente histórico, 4 dígitos)
 * con msws64 de Widynski (estado 64 bits).
 *
 * middle-square por sí solo está roto (ciclos cortos, colapso a cero).
 * msws64 por sí solo es un PRNG decente de 64 bits.
 * El híbrido XOR de ambos produce buena distribución estadística.
 *
 * Uso:
 *   hybrid_prng_t rng;
 *   hybrid_prng_seed(&rng, 1234ULL);
 *   uint64_t r = hybrid_prng_next(&rng);
 *   double d = hybrid_prng_next_double(&rng);
 *
 * Archivos: hybrid_prng.h, hybrid_prng.c, hybrid_prng_test.c, README.md
 */

typedef struct {
    uint64_t ms_state;   /* estado middle-square (0..9999) */
    uint64_t ms_state2;  /* segundo estado middle-square (mezcla) */
    uint64_t mw_state;   /* estado msws64 (64 bits) */
} hybrid_prng_t;

/* Inicializa el generador híbrido con una semilla no nula */
static inline void hybrid_prng_seed(hybrid_prng_t *rng, uint64_t seed) {
    rng->ms_state = (seed % 9999) + 1;
    rng->ms_state2 = ((seed >> 16) % 9999) + 1;
    rng->mw_state = seed | 1ULL;
}

/* Middle-square de von Neumann: 4 dígitos del medio del cuadrado */
static inline uint16_t hybrid_ms_next(uint16_t x) {
    uint32_t sq = (uint32_t)x * (uint32_t)x;
    uint16_t d1 = (uint16_t)((sq / 1000) % 10);
    uint16_t d2 = (uint16_t)((sq / 100)  % 10);
    uint16_t d3 = (uint16_t)((sq / 10)   % 10);
    uint16_t d4 = (uint16_t)( sq         % 10);
    return (uint16_t)(d1 * 1000 + d2 * 100 + d3 * 10 + d4);
}

/* msws64: multiplicación + xorshift (Widynski 2017) */
static inline uint64_t hybrid_msws_next(uint64_t *state) {
    *state *= 6364136223846793005ULL;
    *state ^= *state >> 17;
    *state *= 1442695040888963407ULL;
    *state ^= *state >> 31;
    *state *= 3141592653589793239ULL;
    *state ^= *state >> 27;
    return *state;
}

/* Genera el siguiente valor híbrido de 64 bits */
static inline uint64_t hybrid_prng_next(hybrid_prng_t *rng) {
    uint16_t ms1 = hybrid_ms_next((uint16_t)rng->ms_state);
    uint16_t ms2 = hybrid_ms_next((uint16_t)rng->ms_state2);
    rng->ms_state = ms1;
    rng->ms_state2 = ms2;

    uint64_t mw = hybrid_msws_next(&rng->mw_state);
    uint64_t ms_combined = ((uint64_t)ms1 << 48) | ((uint64_t)ms2 << 32);
    return mw ^ ms_combined;
}

/* Genera un double en [0, 1) */
static inline double hybrid_prng_next_double(hybrid_prng_t *rng) {
    return (double)hybrid_prng_next(rng) / (double)UINT64_MAX;
}

/* Genera un entero en [0, n) usando muestreo por rechazo */
static inline uint64_t hybrid_prng_next_bounded(hybrid_prng_t *rng, uint64_t n) {
    uint64_t x, limit = UINT64_MAX - (UINT64_MAX % n);
    do {
        x = hybrid_prng_next(rng);
    } while (x >= limit);
    return x % n;
}

#endif /* HYBRID_PRNG_H */