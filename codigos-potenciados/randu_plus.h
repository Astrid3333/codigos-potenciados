/* randu_plus.h - RANDU + fmix32 (Murmur3 finalizer) — AVISO IMPORTANTE
 *
 * ESTO NO ES UN MEJORAMIENTO DE RANDU. Es una demostración de por qué no lo es.
 *
 * RANDU histórico: x[n+1] = 65539 * x[n] mod 2^32, multiplicador 65539 = 2^16 + 3.
 * Su defecto de planos (el estado cumple x[n+2] = 6*x[n+1] - 9*x[n] mod 2^32)
 * hace que los puntos caigan en un número reducido de planos del espacio 3D,
 * y por eso falla el test espectral 3D.
 *
 * fmix32 es el finalizador de MurmurHash3. Es una BIYECCIÓN sobre los 32 bits:
 * cada salida tiene un unico estado previo. Por lo tanto:
 *
 *   - fmix32 NO oculta el defecto de planos de RANDU. La salida determina el
 *     estado, y con el estado se recupera el retículo. El test espectral 3D
 *     aplicado a la salida de randu_plus() puede pasar por casualidad del
 *     muestreo, pero la estructura sigue siendo recuperable.
 *   - El periodo NO mejora. El estado interno sigue siendo el LCG de RANDU, y
 *     su periodo es 2^30 (semilla impar), no 2^29 ni 2^32.
 *
 * Ver randu_plus.c: el test comprueba precisamente estas dos afirmaciones
 * (biyectividad y periodo real) y verifica la recuperacion del retículo.
 *
 * Para generar numeros pseudoaleatorios de calidad use, en este mismo repo:
 *   - codigos-potenciados/msws64/   (Widynski 2017, 64 bits, periodo ~2^64)
 *   - codigos-potenciados/hybrid_prng/ (middle-square + msws64)
 * Randu_plus se conserva solo como ejemplo historico y referencia de estudio.
 *
 * Uso:
 *   uint32_t seed = 12345;
 *   uint32_t r = randu_plus(&seed);
 */
#ifndef RANDU_PLUS_H
#define RANDU_PLUS_H

#include <stdint.h>

/* Periodo real del LCG de RANDU con semilla impar (medido, no supuesto). */
#define RANDU_PLUS_PERIOD 1073741824u   /* 2^30 */

/* Inversa de fmix32. Sirve para demostrar que la salida determina el estado. */
static inline uint32_t randu_plus_unfmix32(uint32_t y) {
    y ^= y >> 16;
    y *= 0x7ed1b41du;   /* 0xc2b2ae35^-1 mod 2^32 */
    y ^= y >> 13;
    y ^= y >> 26;       /* inverso completo de xorshift 13 */
    y *= 0xa5cb9243u;   /* 0x85ebca6b^-1 mod 2^32 */
    y ^= y >> 16;
    return y;
}

/* Inversa multiplicativa de 65539 mod 2^32 (para recuperar el estado previo). */
static inline uint32_t randu_plus_inverse_multiplier(void) {
    return 0x1c71aaabu;   /* 65539^-1 mod 2^32 */
}

/* Genera el siguiente valor. El estado se fuerza impar para evitar el
 * subperiodo degenerado de la semilla cero. */
static inline uint32_t randu_plus(uint32_t *state) {
    *state |= 1u;                         /* semilla impar: periodo 2^30 */
    *state = 65539u * *state;              /* RANDU LCG (mod 2^32 implicito) */
    uint32_t x = *state;
    /* fmix32 (MurmurHash3 finalizer): biyectivo, por eso no arregla el LCG */
    x ^= x >> 16;
    x *= 0x85ebca6bu;
    x ^= x >> 13;
    x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}

#endif /* RANDU_PLUS_H */
