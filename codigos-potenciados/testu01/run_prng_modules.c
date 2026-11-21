/* run_prng_modules.c - ejecuta las baterías de TestU01 sobre los generadores
 * de 64 bits del repositorio y sobre xoshiro256** como referencia.
 *
 * Compilar con el Makefile de este directorio. Requiere TestU01 instalado
 * (variable de entorno TESTU01_HOME apuntando al prefijo de instalación).
 *
 * Uso:
 *   run_prng_modules smallcrush [mx64|hybrid|xoshiro] [low|high|split] [semilla]
 *   run_prng_modules crush     [mx64|hybrid|xoshiro] [low|high|split] [semilla]
 *   run_prng_modules bigcrush  [mx64|hybrid|xoshiro] [low|high|split] [semilla]
 *   run_prng_modules digest    [mx64|hybrid|xoshiro] [low|high|split] [semilla]
 *
 * El verbo `digest` no ejecuta ninguna batería: imprime una suma FNV-1a de
 * 1.000.000 de salidas del generador en el modo indicado. Sirve para
 * comprobar empíricamente si dos generadores producen el mismo flujo (por
 * ejemplo, si el modo `low` de `hybrid_prng` coincide con
 * `mult_xorshift64`, porque la mezcla middle-square solo toca la mitad alta).
 *
 * Los modos low/high/split no son estados intermedios del generador: son las
 * tres formas de trocear la palabra de 64 bits para que TestU01 (que trabaja
 * con palabras de 32 bits) pueda examinar el stream. Ver uPrngModulos.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unif01.h"
#include "bbattery.h"
#include "uPrngModulos.h"

static int gen_from_arg (const char *name)
{
   if (strcmp (name, "mx64") == 0)
      return UPM_GEN_MX64;
   if (strcmp (name, "hybrid") == 0)
      return UPM_GEN_HYBRID;
   if (strcmp (name, "xoshiro") == 0)
      return UPM_GEN_XOSHIRO;
   return -1;
}

static int mode_from_arg (const char *name)
{
   if (strcmp (name, "low") == 0)
      return UPM_MODE_LOW;
   if (strcmp (name, "high") == 0)
      return UPM_MODE_HIGH;
   if (strcmp (name, "split") == 0)
      return UPM_MODE_SPLIT;
   return -1;
}

#define UPM_DIGEST_N 1000000

static unsigned long long digest_fnv1a (unif01_Gen *g, long n)
{
   unsigned long long h = 1469598103934665603ULL;
   long i;
   for (i = 0; i < n; ++i) {
      unsigned long bits = g->GetBits (g->param, g->state);
      h ^= (unsigned long long) bits;
      h *= 1099511628211ULL;
   }
   return h;
}

static int run_one (const char *battery, int gen, int mode,
                    unsigned long long seed)
{
   unif01_Gen *g;

   g = uPrngModulos_Create (gen, mode, seed);
   if (g == NULL) {
      fprintf (stderr, "no se pudo crear el generador\n");
      return 1;
   }

   printf ("\n================================================================\n");
   printf ("Bateria      : %s\n", battery);
   printf ("Generador    : %s\n", g->name);
   printf ("Semilla      : %llu\n", seed);
   printf ("================================================================\n");

   if (strcmp (battery, "digest") == 0) {
      unsigned long long h = digest_fnv1a (g, UPM_DIGEST_N);
      printf ("digest       : FNV-1a de %d salidas de 32 bits\n", UPM_DIGEST_N);
      printf ("digest_hex   : %016llx\n", h);
   } else if (strcmp (battery, "smallcrush") == 0) {
      bbattery_SmallCrush (g);
   } else if (strcmp (battery, "crush") == 0) {
      bbattery_Crush (g);
   } else if (strcmp (battery, "bigcrush") == 0) {
      bbattery_BigCrush (g);
   } else {
      fprintf (stderr, "bateria desconocida: %s\n", battery);
      uPrngModulos_Delete (g);
      return 1;
   }

   uPrngModulos_Delete (g);
   return 0;
}

int main (int argc, char **argv)
{
   const char *battery;
   const char *genarg = "mx64";
   const char *modearg = "split";
   unsigned long long seed = 12345ULL;
   int gen, mode;

   if (argc < 2) {
      fprintf (stderr,
               "uso: %s smallcrush|crush|bigcrush "
               "[mx64|hybrid|xoshiro] [low|high|split] [semilla]\n",
               argv[0]);
      return 1;
   }
   battery = argv[1];
   if (argc >= 3)
      genarg = argv[2];
   if (argc >= 4)
      modearg = argv[3];
   if (argc >= 5)
      seed = strtoull (argv[4], NULL, 10);

   gen = gen_from_arg (genarg);
   mode = mode_from_arg (modearg);
   if (gen < 0) {
      fprintf (stderr, "generador desconocido: %s\n", genarg);
      return 1;
   }
   if (mode < 0) {
      fprintf (stderr, "modo desconocido: %s\n", modearg);
      return 1;
   }

   return run_one (battery, gen, mode, seed);
}
