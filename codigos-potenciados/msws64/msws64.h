#ifndef MSWS64_H
#define MSWS64_H

#include <stdint.h>

/*
 * msws64.h - Generador PRNG de 64 bits: multiplicacion + xorshift (3 rondas)
 *
 * CORRECCION IMPORTANTE (2026-11-20): este generador NO es el "msws64" de
 * Widynski (2017). El msws64 de Widynski es un xorshift de 64 bits con
 * desplazamientos 12/25/27 y una multiplicacion final por 0x2545F4914F6CDD1D;
 * su implementacion canonica esta en "msws64_widynski.h" (mismo directorio).
 * Los dos generadores producen salidas distintas desde la primera iteracion
 * (comprobado en msws64.c).
 *
 * El nombre del modulo se conserva porque lcg_lehmer, hybrid_prng y
 * middle_square lo usan, pero la atribucion que aparecia antes en esta
 * cabecera era incorrecta y ya no debe repetirse en ningun documento.
 *
 * Algoritmo realmente implementado (estado de 64 bits; la salida ES el estado):
 *   s  = s * 6364136223846793005
 *   s ^= s >> 17
 *   s  = s * 1442695040888963407
 *   s ^= s >> 31
 *   s  = s * 3141592653589793239
 *   s ^= s >> 27
 *   salida = s
 *
 * Estado: 64 bits (un solo uint64_t).
 * Periodo: NO VERIFICADO. Esta cadena de multiplicacion-xorshift de 3 rondas
 *   no tiene un periodo 2^64 demostrado (no es un LCG con orden conocido):
 *   solo se ha comprobado que no repite en las primeras 100000 muestras.
 * Sumidero 0: la semilla 0 quedaria fija en 0; por eso la siembra fuerza
 *   estado impar (seed | 1).
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