/* uPrngModulos.h - generadores de 64 bits enchufables en TestU01
 *
 * TestU01 trabaja sobre palabras de 32 bits, asi que un generador de 64
 * bits se somete a la bateria de tres formas distintas, y todas son
 * "crudas" en el sentido de que no aplican ningun post-proceso:
 *
 *   UPM_MODE_LOW   - la mitad baja de cada palabra de 64 bits.
 *   UPM_MODE_HIGH  - la mitad alta de cada palabra de 64 bits.
 *   UPM_MODE_SPLIT - las dos mitades en el orden natural (baja, alta).
 *
 * Los tres generadores son:
 *
 *   UPM_GEN_MX64      - mult_xorshift64 (mult_xorshift64/mult_xorshift64.h).
 *   UPM_GEN_HYBRID    - hybrid_prng (hybrid_prng/hybrid_prng.h).
 *   UPM_GEN_XOSHIRO   - xoshiro256** como referencia de calidad conocida.
 *
 * xoshiro256** es la referencia porque es el generador de 64 bits que la
 * literatura clasifica como no lineal y con periodo 2^256: si un generador
 * propio no puede acercarse a sus resultados en SmallCrush, el problema es
 * del generador propio. La implementacion es la del algoritmo publicado por
 * David Blackman y Sebastiano Vigna (xoshiro256starstar.c), con siembra
 * splitmix64; se ha verificado palabra a palabra contra ese puerto.
 *
 * Uso:
 *   unif01_Gen *g = uPrngModulos_Create(UPM_GEN_MX64, UPM_MODE_SPLIT, 1234ULL);
 *   bbattery_SmallCrush(g);
 *   uPrngModulos_Delete(g);
 */
#ifndef UPRNGMODULOS_H
#define UPRNGMODULOS_H

#include "unif01.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Generadores disponibles. */
#define UPM_GEN_MX64    0
#define UPM_GEN_HYBRID  1
#define UPM_GEN_XOSHIRO 2

/* Modo de troceado de la palabra de 64 bits. */
#define UPM_MODE_LOW   0
#define UPM_MODE_HIGH  1
#define UPM_MODE_SPLIT 2

/* Crea un generador de 64 bits para TestU01. */
unif01_Gen *uPrngModulos_Create (int gen, int mode, unsigned long long seed);

/* Libera el generador. */
void uPrngModulos_Delete (unif01_Gen * gen);

/* Escribe el estado actual por stdout (usado por gen->Write). */
void uPrngModulos_Write (void *state);

#ifdef __cplusplus
}
#endif

#endif /* UPRNGMODULOS_H */
