#ifndef MSWS64_H
#define MSWS64_H

#include <stdint.h>

/*
 * msws64.h - Generador PRNG Widynski msws64 (2017)
 *
 * Generador de numeros pseudoaleatorios de 64 bits basado en
 * multiplicacion por constantes impares y desplazamientos xorshift.
 *
 * Estado: 64 bits (un solo uint64_t).
 * Periodo: ~2^64.
 * Velocidad: ~1.8 ns por iteracion.
 *
 * Uso:
 *   msws64_t rng;
 *   msws64_seed(&rng, 1234ULL);
 *   uint64_t r = msws64_next(&rng);
 *   double d = msws64_next_double(&rng);
 */

/* Estado del generador */
typedef struct {
    uint64_t state;
} msws64_t;

/* Inicializa el estado con una semilla no nula */
static inline void msws64_seed(msws64_t *rng, uint64_t seed) {
    rng->state = seed | 1ULL;  /* asegurar que no sea cero */
}

/* Genera el siguiente valor aleatorio de 64 bits */
static inline uint64_t msws64_next(msws64_t *rng) {
    rng->state *= 6364136223846793005ULL;
    rng->state ^= rng->state >> 17;
    rng->state *= 1442695040888963407ULL;
    rng->state ^= rng->state >> 31;
    rng->state *= 3141592653589793239ULL;
    rng->state ^= rng->state >> 27;
    return rng->state;
}

/* Genera un double en el intervalo [0, 1) */
static inline double msws64_next_double(msws64_t *rng) {
    return (double)msws64_next(rng) / (double)UINT64_MAX;
}

/* Genera un entero en [0, n) usando rechazo para evitar sesgo */
static inline uint64_t msws64_next_bounded(msws64_t *rng, uint64_t n) {
    uint64_t x, limit = UINT64_MAX - (UINT64_MAX % n);
    do {
        x = msws64_next(rng);
    } while (x >= limit);
    return x % n;
}

#endif /* MSWS64_H */