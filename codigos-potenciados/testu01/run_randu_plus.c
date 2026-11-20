/* run_randu_plus.c - ejecuta las baterías de TestU01 sobre randu_plus
 *
 * Compilar con el Makefile de este directorio. Requiere TestU01 instalado
 * (variable de entorno TESTU01_HOME apuntando al prefijo de instalación).
 *
 * Uso:
 *   run_randu_plus smallcrush [state|fmix|both] [semilla]
 *   run_randu_plus crush     [state|fmix|both] [semilla]
 *   run_randu_plus bigcrush  [state|fmix|both] [semilla]
 *
 * "both" ejecuta primero el stream del estado crudo del LCG y despues el
 * stream de la salida fmix32, que son los dosUltimos lugares donde puede
 * esconderse el defecto de RANDU.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unif01.h"
#include "bbattery.h"
#include "uRanduPlus.h"

static const char *MODE_NAME[2] = {"estado crudo (RANDU LCG)",
                                   "salida fmix32 (randu_plus)"};

static int run_one (const char *battery, int mode, unsigned long seed)
{
   unif01_Gen *gen;

   gen = uRanduPlus_Create (seed, mode);
   if (gen == NULL) {
      fprintf (stderr, "no se pudo crear el generador\n");
      return 1;
   }

   printf ("\n================================================================\n");
   printf ("Bateria      : %s\n", battery);
   printf ("Modo         : %s\n", MODE_NAME[mode]);
   printf ("Semilla      : %lu\n", seed);
   printf ("================================================================\n");

   if (strcmp (battery, "smallcrush") == 0) {
      bbattery_SmallCrush (gen);
   } else if (strcmp (battery, "crush") == 0) {
      bbattery_Crush (gen);
   } else if (strcmp (battery, "bigcrush") == 0) {
      bbattery_BigCrush (gen);
   } else {
      fprintf (stderr, "bateria desconocida: %s\n", battery);
      uRanduPlus_Delete (gen);
      return 1;
   }

   uRanduPlus_Delete (gen);
   return 0;
}

int main (int argc, char **argv)
{
   const char *battery;
   const char *modearg = "both";
   unsigned long seed = 12345UL;
   int rc = 0;

   if (argc < 2) {
      fprintf (stderr,
               "uso: %s smallcrush|crush|bigcrush [state|fmix|both] [semilla]\n",
               argv[0]);
      return 1;
   }
   battery = argv[1];
   if (argc >= 3)
      modearg = argv[2];
   if (argc >= 4)
      seed = strtoul (argv[3], NULL, 10);

   if (strcmp (modearg, "both") == 0) {
      rc |= run_one (battery, URP_MODE_STATE, seed);
      rc |= run_one (battery, URP_MODE_FMIX, seed);
   } else if (strcmp (modearg, "state") == 0) {
      rc |= run_one (battery, URP_MODE_STATE, seed);
   } else if (strcmp (modearg, "fmix") == 0) {
      rc |= run_one (battery, URP_MODE_FMIX, seed);
   } else {
      fprintf (stderr, "modo desconocido: %s\n", modearg);
      return 1;
   }
   return rc;
}
