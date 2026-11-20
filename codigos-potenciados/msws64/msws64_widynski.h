#ifndef MSWS64_WIDYNSKI_H
#define MSWS64_WIDYNSKI_H

#include <stdint.h>

/*
 * msws64_widynski.h - msws64 de Widynski (2017), implementacion canonica
 *
 * Este es el algoritmo described en M. Widynski, "Middle Square Weyl
 * Sequence PRNG" (2017): un xorshift de 64 bits con desplazamientos 12/25/27
 * y multiplicacion final del estado por 0x2545F4914F6CDD1D.
 *
 * Se incluye porque el modulo "msws64" de este repositorio implementa otro
 * algoritmo (cadena de multiplicacion-xorshift de 3 rondas, ver msws64.h) y
 * ambos producen salidas distintas desde la primera iteracion. Sirve como
 * referencia para comparar, no como reemplazo del modulo existente.
 *
 * Estado: 64 bits (un solo uint64_t). La salida es el estado multiplicado.
 * Sumidero 0: la semilla 0 quedaria fija en 0; la siembra fuerza estado impar.
 *
 * Uso:
 *   uint64_t state = 12345ULL | 1ULL;
 *   uint64_t r = msws64_widynski_next(&state);
 */

static inline void msws64_widynski_seed(uint64_t *state, uint64_t seed) {
    *state = seed | 1ULL;
}

static inline uint64_t msws64_widynski_next(uint64_t *state) {
    uint64_t x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

#endif /* MSWS64_WIDYNSKI_H */
