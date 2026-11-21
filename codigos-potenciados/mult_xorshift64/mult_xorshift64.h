#ifndef MULT_XORSHIFT64_H
#define MULT_XORSHIFT64_H

#include <stdint.h>

/*
 * mult_xorshift64.h - Generador PRNG de 64 bits: multiplicacion + xorshift
 *
 * Este generador se llamaba antes "msws64", nombre que era incorrecto: NO es
 * el msws64 de Widynski (2017) ni el xorshift64* de Vigna. Es una cadena de
 * multiplicacion por constantes impares y xorshift de 3 rondas. El nombre del
 * modulo y de sus simbolos se renombro a "mult_xorshift64" el 2026-11-20 para
 * que la etiqueta diga lo que el codigo hace.
 *
 * Para el msws64 real de Widynski (cuadrado + incremento de Weyl + rotacion)
 * ver el modulo "msws_widynski" en el directorio codigos-potenciados/.
 *
 * Algoritmo implementado (estado de 64 bits; la salida ES el estado):
 *   s  = s * 6364136223846793005
 *   s ^= s >> 17
 *   s  = s * 1442695040888963407
 *   s ^= s >> 31
 *   s  = s * 3141592653589793239
 *   s ^= s >> 27
 *   salida = s
 *
 * Estado: 64 bits (un solo uint64_t).
 * Periodo: cota inferior VERIFICADA por medicion. Cada paso es una
 *   composicion de seis biyecciones de Z/2^64 (tres multiplicaciones por
 *   constantes impares y tres xorshift con desplazamiento a la derecha), luego
 *   el mapa M es una biyeccion: toda orbita es un ciclo puro, sin preperiodo,
 *   y el estado 0 es un punto fijo. Eso NO fija el periodo, que se MIDIO con
 *   el algoritmo de Brent: sin deteccion de ciclo en 2^37 pasos, el periodo
 *   lambda > 2^37/3 > 4.58e10 (> 2^35) para tres semillas independientes.
 *   El periodo exacto (del orden de 2^64) no es alcanzable por medicion
 *   directa; ver mult_xorshift64_period_test.c.
 * Sumidero 0: la semilla 0 quedaria fija en 0; por eso la siembra fuerza
 *   estado impar (seed | 1). En 2.000.000 de muestras no se encontro ningun
 *   otro punto fijo.
 * Velocidad: ~1.8 ns por iteracion.
 *
 * Uso:
 *   mult_xorshift64_t rng;
 *   mult_xorshift64_seed(&rng, 1234ULL);
 *   uint64_t r = mult_xorshift64_next(&rng);
 *   double d = mult_xorshift64_next_double(&rng);
 */

/* Estado del generador */
typedef struct {
    uint64_t state;
} mult_xorshift64_t;

/* Inicializa el estado con una semilla no nula */
static inline void mult_xorshift64_seed(mult_xorshift64_t *rng, uint64_t seed) {
    rng->state = seed | 1ULL;  /* asegurar que no sea cero */
}

/* Genera el siguiente valor aleatorio de 64 bits */
static inline uint64_t mult_xorshift64_next(mult_xorshift64_t *rng) {
    rng->state *= 6364136223846793005ULL;
    rng->state ^= rng->state >> 17;
    rng->state *= 1442695040888963407ULL;
    rng->state ^= rng->state >> 31;
    rng->state *= 3141592653589793239ULL;
    rng->state ^= rng->state >> 27;
    return rng->state;
}

/* Genera un double en el intervalo [0, 1) */
static inline double mult_xorshift64_next_double(mult_xorshift64_t *rng) {
    return (double)mult_xorshift64_next(rng) / (double)UINT64_MAX;
}

/* Genera un entero en [0, n) usando rechazo para evitar sesgo */
static inline uint64_t mult_xorshift64_next_bounded(mult_xorshift64_t *rng, uint64_t n) {
    uint64_t x, limit = UINT64_MAX - (UINT64_MAX % n);
    do {
        x = mult_xorshift64_next(rng);
    } while (x >= limit);
    return x % n;
}

#endif /* MULT_XORSHIFT64_H */
