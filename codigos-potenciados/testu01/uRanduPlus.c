/* uRanduPlus.c - randu_plus como generador enchufable de TestU01
 *
 * Ver uRanduPlus.h para el contrato y la motivacion pedagogica de los dos
 * modos de salida (estado crudo vs salida fmix32).
 */
#include "uRanduPlus.h"

#include "gdef.h"
#include "unif01.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "randu_plus.h"

/*============================== tipos ====================================*/

typedef struct {
   int mode;
} URP_param;

typedef struct {
   uint32_t s;
} URP_state;

/*=========================== funciones base ==============================*/

static unsigned long urp_next_bits (void *vpar, void *vsta)
{
   URP_param *param = (URP_param *) vpar;
   URP_state *state = (URP_state *) vsta;
   uint32_t x;

   /* randu_plus() fuerza semilla impar y devuelve fmix32(estado). */
   x = randu_plus (&state->s);

   if (param->mode == URP_MODE_STATE) {
      /* El estado POST-LCG es la secuencia que RANDU produce. Para obtenerlo
       * hay que deshacer fmix32, pero fmix32 es biyectivo y la salida
       * post-LCG esta disponible antes delmezclado. Como randu_plus()
       * devuelve fmix32(nuevo_estado), deshacemos para recuperar el estado. */
      x = randu_plus_unfmix32 (x);
   }
   /* unif01_StripB espera los 32 bits en los bits bajos del unsigned long
    * (zero-extension), igual que hacen los generadores nativos de TestU01. */
   return (unsigned long) x;
}

static double urp_next_u01 (void *vpar, void *vsta)
{
   unsigned long bits = urp_next_bits (vpar, vsta);
   /* unif01_INV32 = 2^-32; el resultado queda en [0,1). */
   return (double) bits * unif01_INV32;
}

void uRanduPlus_Write (void *vsta)
{
   URP_state *state = (URP_state *) vsta;
   printf (" s = %lu\n", (unsigned long) state->s);
}

/*========================== construccion =================================*/

unif01_Gen *uRanduPlus_Create (unsigned long seed, int mode)
{
   unif01_Gen *gen;
   URP_param *param;
   URP_state *state;
   char name[128];

   if (mode != URP_MODE_STATE && mode != URP_MODE_FMIX) {
      fprintf (stderr, "uRanduPlus_Create: modo invalido %d\n", mode);
      return NULL;
   }
   if (seed > 0xffffffffUL) {
      fprintf (stderr, "uRanduPlus_Create: semilla fuera de rango 32 bits\n");
      return NULL;
   }

   gen = (unif01_Gen *) malloc (sizeof (unif01_Gen));
   param = (URP_param *) malloc (sizeof (URP_param));
   state = (URP_state *) malloc (sizeof (URP_state));
   if (gen == NULL || param == NULL || state == NULL) {
      fprintf (stderr, "uRanduPlus_Create: sin memoria\n");
      free (gen);
      free (param);
      free (state);
      return NULL;
   }

   param->mode = mode;
   state->s = (uint32_t) seed;

   if (mode == URP_MODE_STATE) {
      snprintf (name, sizeof (name),
                "uRanduPlus_Create: RANDU LCG 65539 mod 2^32, s = %lu",
                seed);
   } else {
      snprintf (name, sizeof (name),
                "uRanduPlus_Create: RANDU + fmix32, s = %lu", seed);
   }
   gen->name = (char *) malloc (strlen (name) + 1);
   if (gen->name == NULL) {
      fprintf (stderr, "uRanduPlus_Create: sin memoria para el nombre\n");
      free (gen);
      free (param);
      free (state);
      return NULL;
   }
   strcpy (gen->name, name);

   gen->param = param;
   gen->state = state;
   gen->GetBits = &urp_next_bits;
   gen->GetU01 = &urp_next_u01;
   gen->Write = &uRanduPlus_Write;
   return gen;
}

void uRanduPlus_Delete (unif01_Gen * gen)
{
   if (gen == NULL)
      return;
   free (gen->name);
   free (gen->param);
   free (gen->state);
   free (gen);
}
