/* mult_xorshift64_period_test.c - periodo y biyectividad de mult_xorshift64
 *
 * El mapa de estado de una llamada a mult_xorshift64_next() es la composicion
 * de seis operaciones, todas biyectivas sobre Z/2^64:
 *
 *   x -> x*A            (A impar, multiplicacion biyectiva mod 2^64)
 *   x -> x ^ (x >> 17)  (biyectiva)
 *   x -> x*B            (B impar)
 *   x -> x ^ (x >> 31)  (biyectiva)
 *   x -> x*C            (C impar)
 *   x -> x ^ (x >> 27)  (biyectiva)
 *
 * Por tanto M es una biyeccion de un conjunto finito: toda orbita es un ciclo
 * puro (no hay preperiodo) y el estado 0 es un punto fijo. Eso NO fija el
 * periodo, que depende de la estructura concreta de M; el periodo se mide.
 *
 * Este programa:
 *
 *   1) construye la inversa explicita de M y comprueba M^-1(M(x)) = x sobre
 *      dos millones de valores aleatorios (biyectividad verificable);
 *   2) comprueba que 0 es el unico punto fijo en dos millones de muestras;
 *   3) mide el periodo con el algoritmo de Brent (memoria O(1)).
 *
 * Sobre el limite de deteccion: como M es biyectiva, el preperiodo es 0 y
 * Brent encuentra el ciclo en a lo sumo 3*lambda pasos (el tortugo se
 * reposiciona en potencias de dos, la primera no menor que lambda es < 2*lambda,
 * y desde ahi faltan lambda pasos). Por tanto, si no se detecta ciclo en N
 * pasos, se puede afirmar lambda > N/3.
 *
 * Uso:
 *   mult_xorshift64_period_test [limite_de_pasos]
 *
 * El limite por defecto es 2^28 (unos segundos). Para cotas mas altas:
 *   mult_xorshift64_period_test 137438953472   # 2^37
 */
#include "mult_xorshift64.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define A 6364136223846793005ULL
#define B 1442695040888963407ULL
#define C 3141592653589793239ULL

/* Inverso de un impar c modulo 2^64 por iteracion de Newton. */
static uint64_t inv_odd (uint64_t c)
{
    uint64_t x = c;
    int i;
    for (i = 0; i < 6; ++i)
        x *= 2ULL - c * x;
    return x;
}

/* Inversa de x -> x ^ (x >> s): x = y ^ (y>>s) ^ (y>>2s) ^ ... */
static uint64_t unxorshift_right (uint64_t y, int s)
{
    uint64_t x = y;
    int d = s;
    while (d < 64) {
        x ^= y >> d;
        d += s;
    }
    return x;
}

/* Inversa completa de un paso de next(). */
static uint64_t inv_paso (uint64_t y)
{
    uint64_t s5 = unxorshift_right (y, 27);
    uint64_t s4 = s5 * inv_odd (C);
    uint64_t s3 = unxorshift_right (s4, 31);
    uint64_t s2 = s3 * inv_odd (B);
    uint64_t s1 = unxorshift_right (s2, 17);
    return s1 * inv_odd (A);
}

/* Un paso del generador tal cual esta implementado en el header del repo. */
static uint64_t paso (uint64_t s)
{
    mult_xorshift64_t r;
    r.state = s;
    return mult_xorshift64_next (&r);
}

/* Fuente de valores pseudoaleatorios para las comprobaciones (no es el
 * generador bajo prueba: solo produce los x que se invierten). */
static uint64_t rng_prueba (void)
{
    static uint64_t s = 88172645463325252ULL;
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
}

/* Algoritmo de Brent: detecta el ciclo con memoria O(1).
 * Devuelve la longitud del ciclo, o 0 si no se cerro en `limite` pasos. */
static uint64_t periodo_brent (uint64_t semilla, uint64_t limite)
{
    uint64_t power = 1, lam = 0;
    uint64_t tortuga = semilla;
    uint64_t liebre = paso (semilla);
    uint64_t pasos = 1;

    while (tortuga != liebre) {
        if (power == lam) {
            tortuga = liebre;
            power *= 2;
            lam = 0;
        }
        liebre = paso (liebre);
        lam += 1;
        pasos += 1;
        if (pasos >= limite)
            return 0;
    }
    return lam;
}

int main (int argc, char **argv)
{
    uint64_t limite = (argc > 1) ? strtoull (argv[1], NULL, 10) : (1ULL << 28);
    uint64_t semillas[3];
    uint64_t fallos = 0;
    int i;

    printf ("=== 1) Biyectividad: M^-1(M(x)) == x ===\n");
    for (i = 0; i < 2000000; ++i) {
        uint64_t x = rng_prueba ();
        if (inv_paso (paso (x)) != x) {
            ++fallos;
            if (fallos <= 3)
                printf ("  FALLO en x = %" PRIu64 "\n", x);
        }
    }
    printf ("  2.000.000 valores aleatorios, fallos = %" PRIu64 "\n", fallos);
    printf ("  M(0) = %" PRIu64 " (0 es fijo)\n", paso (0));

    printf ("\n=== 2) Punto fijo unico ===\n");
    {
        uint64_t fijos_extra = 0;
        for (i = 0; i < 2000000; ++i) {
            uint64_t x = rng_prueba ();
            if (x != 0 && paso (x) == x)
                ++fijos_extra;
        }
        printf ("  puntos fijos distintos de 0 en 2.000.000 muestras: %"
                PRIu64 "\n", fijos_extra);
    }

    printf ("\n=== 3) Periodo real (Brent, memoria O(1), limite %" PRIu64
            " pasos) ===\n", limite);
    semillas[0] = 1234ULL;
    semillas[1] = 0x9E3779B97F4A7C15ULL;
    semillas[2] = 0x0123456789ABCDEFULL;
    for (i = 0; i < 3; ++i) {
        uint64_t s = semillas[i] | 1ULL;
        clock_t t0 = clock ();
        uint64_t p = periodo_brent (s, limite);
        double seg = (double) (clock () - t0) / CLOCKS_PER_SEC;
        if (p > 0) {
            printf ("  semilla %-20" PRIu64 " periodo = %" PRIu64 "  (%.1f s)\n",
                    s, p, seg);
        } else {
            printf ("  semilla %-20" PRIu64
                    " sin ciclo detectado en %" PRIu64 " pasos (%.1f s)"
                    " -> periodo > limite/3\n", s, limite, seg);
        }
    }

    return fallos == 0 ? 0 : 1;
}
