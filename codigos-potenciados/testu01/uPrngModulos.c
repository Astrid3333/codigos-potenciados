/* uPrngModulos.c - generadores de 64 bits enchufables en TestU01
 *
 * Ver uPrngModulos.h para el contrato y la motivacion de los tres modos de
 * troceado y del generador de referencia.
 */
#include "uPrngModulos.h"

#include "gdef.h"
#include "unif01.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mult_xorshift64/mult_xorshift64.h"
#include "hybrid_prng/hybrid_prng.h"

/*============================== tipos ====================================*/

typedef struct {
   int gen;
   int mode;
} UPM_param;

typedef struct {
   int pending_valid;
   uint32_t pending;
   int gen;
   int mode;
   mult_xorshift64_t mx;
   hybrid_prng_t hy;
   uint64_t xs[4];
} UPM_state;

/*====================== xoshiro256** (referencia) =========================*/

static uint64_t upm_rotl (uint64_t x, int k)
{
   return (x << k) | (x >> (64 - k));
}

static uint64_t upm_splitmix64 (uint64_t *x)
{
   uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
   z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
   z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
   return z ^ (z >> 31);
}

static void upm_xoshiro_seed (uint64_t *s, unsigned long long seed)
{
   uint64_t x = (uint64_t) seed;
   int i;
   for (i = 0; i < 4; i++)
      s[i] = upm_splitmix64 (&x);
}

static uint64_t upm_xoshiro_next (uint64_t *s)
{
   uint64_t result = upm_rotl (s[1] * 5, 7) * 9;
   uint64_t t = s[1] << 17;
   s[2] ^= s[0];
   s[3] ^= s[1];
   s[1] ^= s[2];
   s[0] ^= s[3];
   s[2] ^= t;
   s[3] = upm_rotl (s[3], 45);
   return result;
}

/*=========================== generacion ==================================*/

static uint64_t upm_next64_dispatch (UPM_param *param, UPM_state *state)
{
   switch (param->gen) {
   case UPM_GEN_MX64:
      return mult_xorshift64_next (&state->mx);
   case UPM_GEN_HYBRID:
      return hybrid_prng_next (&state->hy);
   case UPM_GEN_XOSHIRO:
   default:
      return upm_xoshiro_next (state->xs);
   }
}

static unsigned long upm_next_bits (void *vpar, void *vsta)
{
   UPM_param *param = (UPM_param *) vpar;
   UPM_state *state = (UPM_state *) vsta;
   uint64_t x;

   if (param->mode == UPM_MODE_SPLIT && state->pending_valid) {
      state->pending_valid = 0;
      return (unsigned long) state->pending;
   }

   x = upm_next64_dispatch (param, state);

   switch (param->mode) {
   case UPM_MODE_HIGH:
      return (unsigned long) ((uint32_t) (x >> 32));
   case UPM_MODE_SPLIT:
      state->pending_valid = 1;
      state->pending = (uint32_t) (x >> 32);
      return (unsigned long) ((uint32_t) x);
   case UPM_MODE_LOW:
   default:
      return (unsigned long) ((uint32_t) x);
   }
}

static double upm_next_u01 (void *vpar, void *vsta)
{
   unsigned long bits = upm_next_bits (vpar, vsta);
   return (double) bits * unif01_INV32;
}

void uPrngModulos_Write (void *vsta)
{
    UPM_state *state = (UPM_state *) vsta;
    switch (state->gen) {
    case UPM_GEN_MX64:
       printf (" mx.state = %llu\n", (unsigned long long) state->mx.state);
       break;
    case UPM_GEN_HYBRID:
       printf (" hy.mw_state = %llu\n", (unsigned long long) state->hy.mw_state);
       break;
    case UPM_GEN_XOSHIRO:
    default:
       printf (" xoshiro = %llu %llu %llu %llu\n",
               (unsigned long long) state->xs[0], (unsigned long long) state->xs[1],
               (unsigned long long) state->xs[2], (unsigned long long) state->xs[3]);
       break;
    }
}

/*========================== construccion =================================*/

static const char *upm_gen_name (int gen)
{
   switch (gen) {
   case UPM_GEN_MX64:
      return "mult_xorshift64";
   case UPM_GEN_HYBRID:
      return "hybrid_prng";
   case UPM_GEN_XOSHIRO:
      return "xoshiro256**";
   default:
      return "desconocido";
   }
}

static const char *upm_mode_name (int mode)
{
   switch (mode) {
   case UPM_MODE_LOW:
      return "mitad baja de 32 bits";
   case UPM_MODE_HIGH:
      return "mitad alta de 32 bits";
   case UPM_MODE_SPLIT:
      return "64 bits partidos en dos palabras (baja, alta)";
   default:
      return "desconocido";
   }
}

unif01_Gen *uPrngModulos_Create (int gen, int mode, unsigned long long seed)
{
   unif01_Gen *g;
   UPM_param *param;
   UPM_state *state;
   char name[192];

   if (gen != UPM_GEN_MX64 && gen != UPM_GEN_HYBRID && gen != UPM_GEN_XOSHIRO) {
      fprintf (stderr, "uPrngModulos_Create: generador invalido %d\n", gen);
      return NULL;
   }
   if (mode != UPM_MODE_LOW && mode != UPM_MODE_HIGH && mode != UPM_MODE_SPLIT) {
      fprintf (stderr, "uPrngModulos_Create: modo invalido %d\n", mode);
      return NULL;
   }

   g = (unif01_Gen *) malloc (sizeof (unif01_Gen));
   param = (UPM_param *) malloc (sizeof (UPM_param));
   state = (UPM_state *) malloc (sizeof (UPM_state));
   if (g == NULL || param == NULL || state == NULL) {
      fprintf (stderr, "uPrngModulos_Create: sin memoria\n");
      free (g);
      free (param);
      free (state);
      return NULL;
   }

    memset (state, 0, sizeof (UPM_state));
    param->gen = gen;
    param->mode = mode;
    state->gen = gen;
    state->mode = mode;
    state->pending_valid = 0;
   state->pending = 0;
   mult_xorshift64_seed (&state->mx, (uint64_t) seed);
   hybrid_prng_seed (&state->hy, (uint64_t) seed);
   upm_xoshiro_seed (state->xs, seed);

   snprintf (name, sizeof (name),
             "uPrngModulos_Create: %s, %s, semilla = %llu",
             upm_gen_name (gen), upm_mode_name (mode), seed);
   g->name = (char *) malloc (strlen (name) + 1);
   if (g->name == NULL) {
      fprintf (stderr, "uPrngModulos_Create: sin memoria para el nombre\n");
      free (g);
      free (param);
      free (state);
      return NULL;
   }
   strcpy (g->name, name);

   g->param = param;
   g->state = state;
   g->GetBits = &upm_next_bits;
   g->GetU01 = &upm_next_u01;
   g->Write = &uPrngModulos_Write;
   return g;
}

void uPrngModulos_Delete (unif01_Gen * gen)
{
   if (gen == NULL)
      return;
   free (gen->name);
   free (gen->param);
   free (gen->state);
   free (gen);
}
