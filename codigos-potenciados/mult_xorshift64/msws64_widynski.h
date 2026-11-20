#ifndef MSWS64_WIDYNSKI_H
#define MSWS64_WIDYNSKI_H

#include <stdint.h>

/*
 * msws64_widynski.h - msws64 de Widynski (2017): cuadrado + Weyl + rotacion
 *
 * Estructura del algoritmo segun M. Widynski, "Middle Square Weyl Sequence
 * PRNG" (2017): el estado se elevar al cuadrado, se le suma un incremento de
 * Weyl acumulado y luego se rota 32 bits. NO es un xorshift: el xorshift64*
 * de Vigna (desplazamientos 12/25/27 con multiplicador final
 * 0x2545F4914F6CDD1D) es OTRO generador, distinto y con otro origen.
 *
 * Este puerto C usa el incremento de Weyl
 *   0xB5AD4ECEDA1CE2A9
 * que es el que emplea la implementacion en Octave del repo `octave-mcp`
 * (`middle_square_weyl_tool.py`, `hybrid_prng_tool.py`). El incremento es un
 * parametro del algoritmo: si se dispone de otra referencia (p. ej. la
 * implementacion del autor), basta cambiar `MSWS64_WIDYNSKI_INCREMENT` y las
 * secuencias cambian en consecuencia.
 *
 * Estado: 64 bits (cuadrado del estado) + 64 bits (acumulador de Weyl).
 * Sumidero 0: con estado 0 y acumulador 0 la salida es 0; la siembra usa
 * valores no nulos.
 *
 * Uso:
 *   uint64_t x = 12345ULL, w = 0ULL;
 *   uint64_t r = msws64_widynski_next(&x, &w);
 */

#ifndef MSWS64_WIDYNSKI_INCREMENT
#define MSWS64_WIDYNSKI_INCREMENT 0xB5AD4ECEDA1CE2A9ULL
#endif

static inline void msws64_widynski_seed(uint64_t *x, uint64_t *w, uint64_t seed) {
    *x = seed ? seed : 1ULL;
    *w = 0ULL;
}

static inline uint64_t msws64_widynski_next(uint64_t *x, uint64_t *w) {
    uint64_t v = *x;
    v = v * v;                            /* elevar al cuadrado */
    *w += MSWS64_WIDYNSKI_INCREMENT;       /* incremento de Weyl */
    v += *w;
    v = (v >> 32) | (v << 32);            /* rotacion de 32 bits */
    *x = v;
    return v;
}

#endif /* MSWS64_WIDYNSKI_H */
