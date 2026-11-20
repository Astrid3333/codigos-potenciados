/* lattice_recovery.c - por que un generador puede pasar TestU01 y seguir roto
 *
 * RANDU con estado s[n] = 65539 * s[n-1] mod 2^32 cumple la relacion
 * reticular exacta:
 *
 *     s[n+2] = 6*s[n+1] - 9*s[n]   (mod 2^32)
 *
 * randu_plus() aplica fmix32 (finalizador de MurmurHash3) sobre ese estado.
 * fmix32 es BIYECTIVO, luego:
 *
 *   - el stream de salida tiene el mismo periodo (2^30) y el mismo numero
 *     de estados que el LCG;
 *   - la relacion reticular NO se cumple sobre las salidas, porque el
 *     mezclado no lineal rompe la estructura lineal del reticulo;
 *   - pero la relacion se puede RECUPERAR: randu_plus_unfmix32() deshace
 *     fmix32 y devuelve el estado post-LCG exacto.
 *
 * Las baterias de TestU01 son pruebas de caja negra sobre el stream de
 * salida: no saben que fmix32 es invertible. Por eso un stream fmix32 puede
 * pasar tests espectrales que el stream del estado crudo falla, sin que eso
 * signifique que el generador sea sano.
 *
 * Este programa comprueba, con aritmetica exacta de 32 bits:
 *   1. que randu_plus_unfmix32 es la inversa exacta de fmix32;
 *   2. que el reticulo NO es visible directamente en la salida fmix32;
 *   3. que el reticulo se RECUPERA bit a bit desde esa misma salida.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "randu_plus.h"

#define N 100000u

static uint32_t fmix32 (uint32_t x)
{
   x ^= x >> 16;
   x *= 0x85ebca6bu;
   x ^= x >> 13;
   x *= 0xc2b2ae35u;
   x ^= x >> 16;
   return x;
}

int main (void)
{
   uint32_t state, out, rec;
   uint32_t o1 = 0, o2 = 0;
   uint32_t r1 = 0, r2 = 0;
   unsigned long raw_ok = 0, rec_ok = 0;
   unsigned long inv_errors = 0;
   unsigned long i;
   const unsigned long n = N;

   /* ---------- 1. randu_plus_unfmix32 es la inversa exacta de fmix32 --- */
   for (i = 0; i < 1000000u; ++i) {
      uint32_t y = (uint32_t) (i * 2654435761u);
      if (randu_plus_unfmix32 (fmix32 (y)) != y)
         ++inv_errors;
   }
   printf ("1. inversa fmix32: %lu errores en 1000000 muestras -> %s\n",
           inv_errors, inv_errors == 0 ? "EXACTA" : "FALLA");

   /* ---------- 2 y 3. reticulo sobre salida cruda vs recuperada -------- */
   state = 12345u;
   for (i = 0; i < n; ++i) {
      out = randu_plus (&state);           /* salida fmix32 */
      rec = randu_plus_unfmix32 (out);     /* estado post-LCG exacto */

      if (i >= 2) {
         if (out == (uint32_t) (6u * o1 - 9u * o2))
            ++raw_ok;
         if (rec == (uint32_t) (6u * r1 - 9u * r2))
            ++rec_ok;
      }
      o2 = o1; o1 = out;
      r2 = r1; r1 = rec;
   }

   printf ("2. reticulo sobre la salida fmix32 CRUDA:    %lu/%lu lo cumplen "
           "(%.4f%%)\n", raw_ok, n - 2,
           100.0 * (double) raw_ok / (double) (n - 2));
   printf ("3. reticulo sobre el estado RECUPERADO:      %lu/%lu lo cumplen "
           "(%.4f%%)\n", rec_ok, n - 2,
           100.0 * (double) rec_ok / (double) (n - 2));

   printf ("\nConclusion: la relacion reticular de RANDU es RECUPERABLE bit a "
           "bit desde la salida\nfmix32. Un test de caja negra como los de "
           "TestU01 sobre el stream de salida\nno puede verla; pasar esas "
           "pruebas no certifica la salud del generador.\n");

   return (inv_errors == 0 && rec_ok == n - 2) ? 0 : 1;
}
