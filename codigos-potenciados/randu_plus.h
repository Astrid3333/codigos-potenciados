/* randu_plus.h - RANDU + fmix32 (Murmur3 finalizer)
 *
 * Reemplazo directo de RANDU histórico (x_{n+1} = 65539 * x_n mod 2^32)
 * con un mezclador de bits (fmix32) que oculta estadísticamente el defecto de planos
   (el estado interno cumple x[n+2] = 6·x[n+1] − 9·x[n] mod 2^32 y es recuperable
   desde la salida; uso no criptográfico y estadístico está bien).
 *
 * Uso:
 *   uint32_t seed = 12345;
 *   uint32_t r = randu_plus(&seed);
 */
#ifndef RANDU_PLUS_H
#define RANDU_PLUS_H

#include <stdint.h>

static inline uint32_t randu_plus(uint32_t *state) {
    *state |= 1;                          // Garantizar semilla impar, evitar período degenerado
    *state = 65539u * *state;           // RANDU LCG (mod 2^32 implícito en 32-bit)
    uint32_t x = *state;
    // fmix32 (MurmurHash3 finalizer)
    x ^= x >> 16;
    x *= 0x85ebca6bu;
    x ^= x >> 13;
    x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}

#endif /* RANDU_PLUS_H */