/* uRanduPlus.h - randu_plus como generador enchufable de TestU01
 *
 * TestU01 (L'Ecuyer & Simard) permite registrar generadores propios mediante
 * un unif01_Gen con punteros GetU01 / GetBits. Este adaptador expone
 * randu_plus() de randu_plus.h en ese formato.
 *
 * Dos modos de salida, porque son equivalentes en cardinalidad pero NO en
 * distribucion geometrica:
 *
 *   URP_MODE_STATE  - devuelve el estado crudo del LCG de RANDU
 *                      (65539 * s mod 2^32). Es el stream que RANDU genera.
 *   URP_MODE_FMIX   - devuelve la salida de randu_plus() (fmix32 sobre el
 *                      estado). fmix32 es BIYECTIVO, luego el stream tiene el
 *                      mismo periodo y el mismo numero de estados, pero la
 *                      relacion reticular x[n+2] = 6x[n+1] - 9x[n] ya no se
 *                      cumple sobre las salidas: la avlanche no lineal rompe
 *                      el reticulo visible. El reticulo sigue siendo
 *                      RECUPERABLE porque fmix32 se puede invertir
 *                      (randu_plus_unfmix32).
 *
 * Eso es exactamente lo que las baterías de TestU01 pueden y no pueden ver:
 * son pruebas de caja negra sobre el stream de salida.
 *
 * Uso:
 *   unif01_Gen *g = uRanduPlus_Create(12345u, URP_MODE_FMIX);
 *   bbattery_SmallCrush(g);
 *   uRanduPlus_Delete(g);
 */
#ifndef URANDUPLUS_H
#define URANDUPLUS_H

#include "unif01.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Modo de salida del adaptador. */
#define URP_MODE_STATE 0
#define URP_MODE_FMIX  1

/* Crea un generador randu_plus para TestU01.
 * seed  : semilla inicial (se fuerza impar internamente, como randu_plus).
 * mode  : URP_MODE_STATE o URP_MODE_FMIX.
 */
unif01_Gen *uRanduPlus_Create (unsigned long seed, int mode);

/* Libera el generador. */
void uRanduPlus_Delete (unif01_Gen * gen);

/* Escribe el estado actual por stdout (usado por gen->Write). */
void uRanduPlus_Write (void *state);

#ifdef __cplusplus
}
#endif

#endif /* URANDUPLUS_H */
