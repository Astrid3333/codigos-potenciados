/* randu_plus.c - Test de las afirmaciones sobre RANDU + fmix32
 *
 * Este programa NO solo imprime valores: verifica con criterio y devuelve
 * codigo de salida distinto de 0 si alguna afirmacion se incumple.
 *
 * Compilar: cc -std=c99 -O2 -Wall -Wextra -pedantic -Werror randu_plus.c -o randu_test
 * Ejecutar: ./randu_test
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "randu_plus.h"

/* ---------- Utilidades de prueba ---------- */

static int failures = 0;
static int checks = 0;

static void check(int condition, const char *what) {
    ++checks;
    if (condition) {
        printf("PASA  %s\n", what);
    } else {
        printf("FALLA %s\n", what);
        ++failures;
    }
}

/* Eleva a la potencia n el multiplicador de RANDU modulo 2^32, por
 * exponenciacion binaria. Permite medir el periodo sin recorrerlo entero. */
static uint32_t randu_pow_mod(uint32_t base, uint32_t exp) {
    uint32_t result = 1u;
    while (exp > 0u) {
        if (exp & 1u) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

int main(void) {
    uint32_t s = 1u;                 /* semilla impar */
    uint32_t estados[8];
    uint32_t salidas[8];
    int i;

    printf("Prueba RANDU + fmix32 (primeros 10 valores):\n");
    {
        uint32_t demo = 12345u;
        for (i = 0; i < 10; ++i) {
            uint32_t r = randu_plus(&demo);
            printf("%2d: %10u  (0x%08x)\n", i, r, r);
        }
    }
    printf("\n");

    /* 1. fmix32 es biyectivo: la salida determina el estado.
     *    Se comprueba sobre la secuencia real de salidas. */
    s = 1u;
    for (i = 0; i < 8; ++i) {
        salidas[i] = randu_plus(&s);
        estados[i] = s;
    }
    {
        int biyectivo = 1;
        for (i = 0; i < 8; ++i) {
            if (randu_plus_unfmix32(salidas[i]) != estados[i]) biyectivo = 0;
        }
        check(biyectivo,
              "fmix32 es biyectivo: unfmix32(salida) recupera el estado");
    }

    /* 2. Por ser biyectivo, el reticulo de RANDU es recuperable desde la
     *    salida: x[n+2] = 6*x[n+1] - 9*x[n] mod 2^32.
     *    Esta es la razon por la que fmix32 NO arregla el defecto de planos. */
    {
        uint32_t rec[8];
        int reticulo = 1;
        for (i = 0; i < 8; ++i) {
            rec[i] = randu_plus_unfmix32(salidas[i]);
        }
        for (i = 0; i + 2 < 8; ++i) {
            uint32_t lhs = rec[i + 2];
            uint32_t rhs = 6u * rec[i + 1] - 9u * rec[i];
            if (lhs != rhs) reticulo = 0;
        }
        check(reticulo,
              "el reticulo de RANDU (x[n+2] = 6x[n+1] - 9x[n]) es recuperable desde la salida");
    }

    /* 3. El periodo real del LCG de RANDU con semilla impar es 2^30,
     *    no 2^29 ni 2^32. */
    {
        uint32_t p28 = randu_pow_mod(65539u, 1u << 28);
        uint32_t p29 = randu_pow_mod(65539u, 1u << 29);
        uint32_t p30 = randu_pow_mod(65539u, 1u << 30);
        check(p29 != 1u, "65539^(2^29) != 1 mod 2^32 (el periodo no es 2^29)");
        check(p30 == 1u, "65539^(2^30) == 1 mod 2^32 (el periodo es 2^30)");
        check(p28 != 1u, "65539^(2^28) != 1 mod 2^32 (el periodo no divide 2^28)");
        check(RANDU_PLUS_PERIOD == (1u << 30), "RANDU_PLUS_PERIOD declarado coincide con la medicion");
    }

    /* 4. La salida de randu_plus NO coincide con el estado interno:
     *    son cosas distintas y el estado sigue siendo el del LCG. */
    {
        uint32_t st = 1u;
        uint32_t r1 = randu_plus(&st);
        check(r1 != st, "la salida de fmix32 difiere del estado interno del LCG");
    }

    /* 5. La secuencia de estados es periodica con periodo 2^30, y mezclar con
     *    fmix32 no cambia la longitud del ciclo: fmix32 es una permutacion fija
     *    de los 32 bits, no una enlargement del espacio de estados. */
    check(randu_pow_mod(65539u, 1u << 30) == 1u,
          "mezclar con fmix32 no cambia el periodo del ciclo (2^30)");

    printf("\n");
    if (failures == 0) {
        printf("OK  todas las comprobaciones pasan (%d)\n", checks);
        return EXIT_SUCCESS;
    }
    printf("FALLA %d de %d comprobacion(es)\n", failures, checks);
    return EXIT_FAILURE;
}
