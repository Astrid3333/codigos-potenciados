/* middle_square_test.c - Pruebas del modulo middle_square
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra middle_square.c middle_square_test.c -lm -o prueba
 * Ejecutar: ./prueba
 */

#include "middle_square.h"
#include "../mult_xorshift64/mult_xorshift64.h"
#include <stdio.h>
#include <string.h>

/* Referencia: copia literal de la funcion estatica ms_next_4 de
 * ../mult_xorshift64/mult_xorshift64.c (aqui no es exportada) para comprobar equivalencia */
static uint16_t ms_next_4(uint16_t x) {
    uint32_t sq = (uint32_t)x * (uint32_t)x;
    char s[12];
    snprintf(s, sizeof(s), "%08u", sq);
    return (uint16_t)((s[2] - '0') * 1000 + (s[3] - '0') * 100 +
                       (s[4] - '0') * 10 + (s[5] - '0'));
}

static int pruebas_totales = 0;
static int pruebas_aprobadas = 0;

static void verificar(const char *nombre, int aprobado) {
    pruebas_totales++;
    if (aprobado) {
        pruebas_aprobadas++;
        printf("  [OK]  %s\n", nombre);
    } else {
        printf("  [FAIL] %s\n", nombre);
    }
}

static void verificar_entero(const char *nombre, unsigned long long valor,
                             unsigned long long esperado) {
    verificar(nombre, valor == esperado);
}

/* Chi-cuadrado de Pearson sobre cuentas de un histograma */
static double chi_cuadrado(const unsigned long long cuentas[], unsigned int bins,
                           unsigned long long total) {
    double esperado = (double)total / (double)bins;
    double chi2 = 0.0;
    unsigned int i;
    for (i = 0U; i < bins; ++i) {
        double d = (double)cuentas[i] - esperado;
        chi2 += d * d / esperado;
    }
    return chi2;
}

int main(void) {
    /* Secuencia medida de la semilla 1234 (verificada con el criterio de
     * mult_xorshift64.c: 4 digitos centrales del cuadrado con 8 digitos) */
    static const unsigned int esperada_1234[] = {
        1234, 5227, 3215, 3362, 3030, 1809, 2724, 4201, 6484, 422, 1780, 1684,
        8358, 8561, 2907, 4506, 3040, 2416, 8370, 569, 3237, 4781, 8579, 5992,
        9040, 7216, 706, 4984, 8402, 5936, 2360, 5696, 4444, 7491, 1150, 3225,
        4006, 480, 2304, 3084, 5110, 1121, 2566, 5843, 1406, 9768, 4138, 1230,
        5129, 3066, 4003, 240, 576, 3317, 24, 5, 0
    };
    static const unsigned int esperada_101[] = {2100, 4100, 8100, 6100};

    middle_square_t rng;
    uint32_t secuencia[128];
    unsigned long long cuentas_ms[10];
    unsigned long long cuentas_von[10];
    unsigned long long suma_ms = 0ULL;
    unsigned long long suma_von = 0ULL;
    size_t escritos;
    size_t i;
    unsigned int sumideros = 0U;
    unsigned int max_ciclo = 0U;

    printf("=================================================================\n");
    printf("  PRUEBAS - MIDDLE SQUARE (VON NEUMANN) DE 4 DIGITOS\n");
    printf("=================================================================\n\n");

    printf("--- Secuencia de la semilla 1234 ---\n");
    middle_square_semilla(&rng, 1234U);
    escritos = middle_square_secuencia(1234U, secuencia, 128U);
    verificar("la secuencia tiene la longitud medida", escritos ==
        sizeof(esperada_1234) / sizeof(esperada_1234[0]));
    {
        int coincide = 1;
        size_t total = sizeof(esperada_1234) / sizeof(esperada_1234[0]);
        if (escritos != total) {
            coincide = 0;
        }
        for (i = 0U; i < total && i < escritos; ++i) {
            if (secuencia[i] != (uint32_t)esperada_1234[i]) {
                coincide = 0;
            }
        }
        verificar("los 63 estados coinciden con la referencia medida", coincide);
    }
    {
        int coincide = 1;
        uint16_t x = 1234U;
        for (i = 0U; i < escritos; ++i) {
            if (secuencia[i] != (uint32_t)x) {
                coincide = 0;
            }
            x = ms_next_4(x);
        }
        verificar("el paso coincide con ms_next_4 de mult_xorshift64.c", coincide);
    }
    verificar("la semilla 1234 termina en el sumidero 0",
              secuencia[escritos - 1U] == 0U);

    printf("\n--- Paso y rango ---\n");
    middle_square_semilla(&rng, 1234U);
    verificar_entero("el primer paso de 1234 es 5227", middle_square_siguiente(&rng), 5227ULL);
    middle_square_semilla(&rng, 0U);
    verificar_entero("0 es un punto fijo", middle_square_siguiente(&rng), 0ULL);
    verificar_entero("0 sigue siendo 0", middle_square_siguiente(&rng), 0ULL);
    {
        int en_rango = 1;
        middle_square_semilla(&rng, 777U);
        for (i = 0U; i < 1000U; ++i) {
            double d = middle_square_siguiente_double(&rng);
            if (d < 0.0 || d >= 1.0) {
                en_rango = 0;
            }
        }
        verificar("los doubles quedan en [0, 1)", en_rango);
    }
    {
        int acotado = 1;
        middle_square_semilla(&rng, 31337U);
        for (i = 0U; i < 5000U; ++i) {
            uint32_t v = middle_square_siguiente_acotado(&rng, 7U);
            if (v >= 7U) {
                acotado = 0;
            }
        }
        verificar("el valor acotado queda en [0, 7)", acotado);
    }

    printf("\n--- Ciclos ---\n");
    middle_square_semilla(&rng, 6100U);
    verificar_entero("desde dentro del ciclo, el periodo es 4",
                     middle_square_periodo(&rng, 1000U), 4ULL);
    verificar_entero("el ciclo de la semilla 101 es el medido",
                     middle_square_periodo_ciclo(101U, 1000U), 4ULL);
    {
        int coincide = 1;
        middle_square_semilla(&rng, 6100U);
        for (i = 0U; i < sizeof(esperada_101) / sizeof(esperada_101[0]); ++i) {
            if (middle_square_siguiente(&rng) != (uint32_t)esperada_101[i]) {
                coincide = 0;
            }
        }
        verificar("el ciclo 6100 -> 2100 -> 4100 -> 8100 se reproduce", coincide);
    }
    {
        unsigned int semilla;
        for (semilla = 0U; semilla < MIDDLE_SQUARE_TAMANO; ++semilla) {
            unsigned int ciclo = middle_square_periodo_ciclo(semilla, 1000U);
            if (ciclo > max_ciclo) {
                max_ciclo = ciclo;
            }
            if (ciclo == 1U) {
                /* ciclo de longitud 1: se comprueba si el punto fijo es 0 */
                uint32_t x = semilla;
                unsigned int k;
                for (k = 0U; k < 1000U; ++k) {
                    x = (x * x / 100U) % MIDDLE_SQUARE_TAMANO;
                    if (x == 0U) {
                        break;
                    }
                }
                if (x == 0U) {
                    sumideros++;
                }
            }
        }
        verificar("el ciclo maximo del generador es 4", max_ciclo == 4U);
        verificar("1968 de las 10000 semillas caen en el sumidero 0",
                  sumideros == 1968U);
    }

    printf("\n--- Comparacion con mult_xorshift64 ---\n");
    memset(cuentas_ms, 0, sizeof(cuentas_ms));
    memset(cuentas_von, 0, sizeof(cuentas_von));
    {
        /* generador de 64 bits: 100000 muestras */
        mult_xorshift64_t msws;
        unsigned int j;
        mult_xorshift64_seed(&msws, 20261113ULL);
        for (j = 0U; j < 100000U; ++j) {
            uint64_t v = mult_xorshift64_next(&msws);
            /* los 32 bits altos se escalan a 10 bins sin sesgo */
            unsigned int bin = (unsigned int)((((uint64_t)(v >> 32)) * 10ULL) >> 32);
            cuentas_ms[bin % 10U]++;
        }
        suma_ms = 100000ULL;
    }
    {
        /* middle square: una muestra por cada estado posible */
        unsigned int estado;
        for (estado = 0U; estado < MIDDLE_SQUARE_TAMANO; ++estado) {
            uint32_t v = (estado * estado / 100U) % MIDDLE_SQUARE_TAMANO;
            cuentas_von[v / 1000U]++;
        }
        suma_von = (unsigned long long)MIDDLE_SQUARE_TAMANO;
    }
    {
        double chi_ms = chi_cuadrado(cuentas_ms, 10U, suma_ms);
        double chi_von = chi_cuadrado(cuentas_von, 10U, suma_von);
        /* valor critico de 9 grados de libertad al 99%: 21.67 */
        verificar("mult_xorshift64 se ve uniforme (chi2 < 21.67)", chi_ms < 21.67);
        verificar("el mapa middle-square tiene sesgo de cuadrados (chi2 > 21.67)",
                  chi_von > 21.67);
        printf("        chi2 mult_xorshift64 = %.2f | chi2 middle-square = %.2f\n",
               chi_ms, chi_von);
    }
    {
        /* misma media, distinta dispersion: mult_xorshift64 recorre todo el rango */
        mult_xorshift64_t msws;
        double minimo = 1.0;
        double maximo = 0.0;
        unsigned int j;
        mult_xorshift64_seed(&msws, 99ULL);
        for (j = 0U; j < 1000U; ++j) {
            double d = mult_xorshift64_next_double(&msws);
            if (d < minimo) {
                minimo = d;
            }
            if (d > maximo) {
                maximo = d;
            }
        }
        verificar("mult_xorshift64 recorre todo el intervalo [0, 1)",
                  minimo < 0.01 && maximo > 0.99);
    }

    printf("\n=================================================================\n");
    printf("  %d/%d pruebas pasaron\n", pruebas_aprobadas, pruebas_totales);
    printf("=================================================================\n");

    return (pruebas_aprobadas == pruebas_totales) ? 0 : 1;
}
