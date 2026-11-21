/* msws_widynski_test.c - Pruebas del modulo msws_widynski
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra msws_widynski_test.c -o prueba
 * Ejecutar: ./prueba
 *
 * Las secuencias de referencia usadas en estas pruebas se congelaron desde dos
 * implementaciones externas INDEPENDIENTES, nunca desde este modulo:
 *   - la implementacion de referencia del autor (Bernard Widynski), version 5
 *     (mswsrngv5: msws32.h y msws64.h), GPLv3;
 *   - el crate Rust "msws" (twe4ked/msws) para msws32;
 *   - el crate Rust "weyl" (tidwall/weyl-rs, licencia ISC) para la variante
 *     mono de 64 bits.
 * Los vectores del crate "msws" coinciden bit a bit con los de la
 * implementacion del autor, de modo que son dos oraculos concordantes.
 */

#include "msws_widynski.h"
#include "../mult_xorshift64/mult_xorshift64.h"
#include <stdio.h>
#include <string.h>

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

/* ------------------------------------------------------------------ */
/* Vectores congelados de fuentes externas                             */
/* ------------------------------------------------------------------ */

/* msws32: implementacion de referencia del autor v5, verificada ademas
 * bit a bit contra el crate Rust "msws" (twe4ked/msws).
 * s = 0xb5ad4eceda1ce2a9, x = 0, w = 0. */
static const uint32_t esperado_msws32[12] = {
    0xb5ad4eceU, 0xdf4ee85cU, 0x1889155fU, 0xc6dcbccfU, 0x1106e0c5U, 0x473066aeU,
    0x374ac427U, 0x21e9e9bfU, 0x7294da52U, 0x212dbe1aU, 0x9c449b2aU, 0xa2b9f2fcU
};

/* msws64 de dos carriles: implementacion de referencia del autor v5.
 * s1 = 0xb5ad4eceda1ce2a9, s2 = 0x278c5a4d8419fe6b, x = 0, w = 0. */
static const uint64_t esperado_msws64[12] = {
    0x31b4b0a5fd90b8e4ULL, 0xbd08dfa36824fe79ULL, 0xf895643d60a55706ULL,
    0x644e3a9a663281d9ULL, 0x642616e2d6d74b35ULL, 0x5c3ac8f51a20305cULL,
    0xd90648f19f25d755ULL, 0xb1788103945cec2fULL, 0xc9d91df5596c0239ULL,
    0x08c4143ddfa78f4cULL, 0x8adc76f7bafd82c3ULL, 0xe6cd28e8892aafc8ULL
};

/* Variante mono de 64 bits: crate Rust "weyl" (tidwall/weyl-rs, ISC).
 * Semilla -> s = (semilla << 1) + 0xb5ad4eceda1ce2a9. */
static const uint64_t esperado_weyl[4][4] = {
    {0xda1ce2a9b5ad4eceULL, 0x9ae7f316df4ee85cULL, 0x414d890b1889155fULL,
     0x19ea43e5c6dcbccfULL},
    {0xda1ce2abb5ad4eceULL, 0x9ae7f31ab6042394ULL, 0x1fe87591bbec0124ULL,
     0x1ad4d7bcc44c1140ULL},
    {0xda1ce2adb5ad4eceULL, 0x9ae7f31e8cb95eccULL, 0xd4491a97d7f4b225ULL,
     0xa0f3040de4d5d3ffULL},
    {0xda1d431bb5ad4eceULL, 0x9ae8b3faa8c397d4ULL, 0x755b90e1f4e03b33ULL,
     0x4765989529f4e022ULL}
};
static const uint64_t semillas_weyl[4] = {0ULL, 1ULL, 2ULL, 12345ULL};

int main(void) {
    msws32_widynski_t r32;
    msws64_widynski_t r64;
    msws64_mono_widynski_t rmono;
    size_t i;

    printf("=================================================================\n");
    printf("  PRUEBAS - MSWS (MIDDLE SQUARE WEYL SEQUENCE) DE WIDYNSKI\n");
    printf("=================================================================\n\n");

    printf("--- msws32: 32 bits (referencia externa del autor + crate Rust) ---\n");
    msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
    {
        int coincide = 1;
        for (i = 0U; i < 12U; ++i) {
            uint32_t v = msws32_widynski_next(&r32);
            if (v != esperado_msws32[i]) {
                coincide = 0;
                printf("        salida %u esperada %08x, obtenida %08x\n",
                       (unsigned)i, esperado_msws32[i], v);
            }
        }
        verificar("los 12 vectores de msws32 coinciden con las fuentes externas",
                  coincide);
    }

    printf("\n--- msws64: dos carriles, 64 bits (referencia externa del autor) ---\n");
    msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                          MSWS64_WIDYNSKI_S2_PAPER);
    {
        int coincide = 1;
        for (i = 0U; i < 12U; ++i) {
            uint64_t v = msws64_widynski_next(&r64);
            if (v != esperado_msws64[i]) {
                coincide = 0;
                printf("        salida %u esperada %016llx obtained %016llx\n",
                       (unsigned)i, (unsigned long long)esperado_msws64[i],
                       (unsigned long long)v);
            }
        }
        verificar("los 12 vectores de msws64 coinciden con la referencia externa",
                  coincide);
    }

    printf("\n--- Variante mono de 64 bits (crate Rust weyl-rs) ---\n");
    {
        int coincide = 1;
        unsigned int k;
        for (k = 0U; k < 4U; ++k) {
            msws64_mono_widynski_seed(&rmono, semillas_weyl[k]);
            for (i = 0U; i < 4U; ++i) {
                uint64_t v = msws64_mono_widynski_next(&rmono);
                if (v != esperado_weyl[k][i]) {
                    coincide = 0;
                    printf("        semilla %llu salida %u esperada %016llx, obtenida %016llx\n",
                           (unsigned long long)semillas_weyl[k], (unsigned)i,
                           (unsigned long long)esperado_weyl[k][i],
                           (unsigned long long)v);
                }
            }
        }
        verificar("los 16 vectores de la variante mono coinciden con weyl-rs",
                  coincide);
    }

    printf("\n--- Ejemplo del articulo (seccion 3) ---\n");
    {
        /* El articulo toma x = 0xE3296D171EC4A36F, lo eleva al cuadrado
         * (mod 2^64) y extrae el centro desplazando 32 bits a la derecha:
         * el resultado publicado es 0x31C2914A. Se reproduce aqui con el
         * generador completo: se fija x y se deja w = -s para que el
         * incremento de Weyl anulado aporte 0 al cuadrado. */
        msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
        r32.x = 0xE3296D171EC4A36FULL;
        r32.w = 0ULL - r32.s;
        verificar_entero("el ejemplo del articulo produce 31C2914A",
                         (unsigned long long)msws32_widynski_next(&r32),
                         0x31C2914AULL);
    }

    printf("\n--- Consistencia interna ---\n");
    {
        /* El carril 1 de msws64 evoluciona exactamente igual que msws32 con
         * el mismo s, por lo que sus estados internos deben coincidir. */
        int coincide = 1;
        msws32_widynski_seed(&r32, MSWS64_WIDYNSKI_S1_PAPER);
        msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                              MSWS64_WIDYNSKI_S2_PAPER);
        for (i = 0U; i < 1000U; ++i) {
            (void)msws32_widynski_next(&r32);
            (void)msws64_widynski_next(&r64);
            if (r32.x != r64.x1 || r32.w != r64.w1) {
                coincide = 0;
                break;
            }
        }
        verificar("el carril 1 de msws64 sigue el mismo estado que msws32",
                  coincide);
    }
    {
        /* Salida de msws64 = (x1 sin rotar) XOR (x2 rotado). Como la
         * rotacion es involutiva, x1 sin rotar se recupera rotando x1. */
        int coincide = 1;
        msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                              MSWS64_WIDYNSKI_S2_PAPER);
        for (i = 0U; i < 1000U; ++i) {
            uint64_t salida = msws64_widynski_next(&r64);
            uint64_t esperado = msws_widynski_rot(r64.x1) ^ r64.x2;
            if (salida != esperado) {
                coincide = 0;
                break;
            }
        }
        verificar("la salida de msws64 cumple xx XOR x2 con los estados internos",
                  coincide);
    }
    {
        /* La mitad baja de la variante mono coincide con la salida de msws32
         * para el mismo s, en ambos generadores y durante 1000 pasos. */
        int coincide = 1;
        msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
        msws64_mono_widynski_seed_const(&rmono, MSWS32_WIDYNSKI_S_PAPER);
        for (i = 0U; i < 1000U; ++i) {
            uint32_t a = msws32_widynski_next(&r32);
            uint64_t b = msws64_mono_widynski_next(&rmono);
            if (a != (uint32_t)(b & 0xFFFFFFFFULL)) {
                coincide = 0;
                break;
            }
        }
        verificar("la mitad baja de la variante mono coincide con msws32",
                  coincide);
    }
    {
        /* Control negativo: msws64 de dos carriles NO es el generador
         * mult_xorshift64 del repo (que es multiplicacion + xorshift, no
         * cuadrado + Weyl + rotacion). */
        mult_xorshift64_t mx;
        int diferencia = 0;
        msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                              MSWS64_WIDYNSKI_S2_PAPER);
        mult_xorshift64_seed(&mx, MSWS64_WIDYNSKI_S1_PAPER);
        for (i = 0U; i < 1000U; ++i) {
            if (msws64_widynski_next(&r64) != mult_xorshift64_next(&mx)) {
                diferencia = 1;
                break;
            }
        }
        verificar("msws64 de dos carriles es distinto de mult_xorshift64",
                  diferencia);
    }

    printf("\n--- Salto adelante (jump ahead) ---\n");
    {
        /* Propiedad matematica: el acumulador de Weyl queda exactamente
         * m * s por delante (Teorema A del articulo). */
        const uint64_t m = 1000000000ULL;
        msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_jump_ahead(&r32, m);
        verificar_entero("el salto avanza w en m * s (mod 2^64)",
                         (unsigned long long)r32.w,
                         (unsigned long long)(m * MSWS32_WIDYNSKI_S_PAPER));
    }
    {
        const uint64_t m = 1000000000ULL;
        msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                              MSWS64_WIDYNSKI_S2_PAPER);
        msws64_widynski_jump_ahead(&r64, m);
        verificar_entero("el salto de msws64 avanza w1 en m * s1",
                         (unsigned long long)r64.w1,
                         (unsigned long long)(m * MSWS64_WIDYNSKI_S1_PAPER));
        verificar_entero("el salto de msws64 avanza w2 en m * s2",
                         (unsigned long long)r64.w2,
                         (unsigned long long)(m * MSWS64_WIDYNSKI_S2_PAPER));
    }
    {
        /* Flujos distintos: saltar 0, 1 y 2 veces debe dar tres secuencias
         * diferentes, y ninguna debe coincidir con el flujo original. */
        msws32_widynski_t a;
        msws32_widynski_t b;
        msws32_widynski_t c;
        int distintos = 1;
        uint32_t va[4];
        uint32_t vb[4];
        uint32_t vc[4];
        unsigned int k;

        msws32_widynski_seed(&a, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_seed(&b, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_seed(&c, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_jump_ahead(&b, 1ULL);
        msws32_widynski_jump_ahead(&c, 2ULL);

        for (k = 0U; k < 4U; ++k) {
            va[k] = msws32_widynski_next(&a);
            vb[k] = msws32_widynski_next(&b);
            vc[k] = msws32_widynski_next(&c);
            if (va[k] == vb[k] || va[k] == vc[k] || vb[k] == vc[k]) {
                distintos = 0;
            }
        }
        verificar("los flujos con saltos 0, 1 y 2 son distintos", distintos);
    }
    {
        /* Tras el salto, el generador sigue produciendo una secuencia
         * utilizable (no se degenera en un valor constante). */
        msws32_widynski_t a;
        int variety = 0;
        unsigned int k;
        uint32_t v0;
        msws32_widynski_seed(&a, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_jump_ahead(&a, 1000000000ULL);
        v0 = msws32_widynski_next(&a);
        for (k = 0U; k < 1000U; ++k) {
            if (msws32_widynski_next(&a) != v0) {
                variety = 1;
                break;
            }
        }
        verificar("el generador sigue vivo despues del salto", variety);
    }

    printf("\n--- Rangos ---\n");
    {
        int en_rango = 1;
        msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
        for (i = 0U; i < 10000U; ++i) {
            double d = msws32_widynski_next_double(&r32);
            if (d < 0.0 || d >= 1.0) {
                en_rango = 0;
            }
        }
        verificar("los doubles de msws32 quedan en [0, 1)", en_rango);
    }
    {
        int en_rango = 1;
        msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                              MSWS64_WIDYNSKI_S2_PAPER);
        for (i = 0U; i < 10000U; ++i) {
            double d = msws64_widynski_next_double(&r64);
            if (d < 0.0 || d >= 1.0) {
                en_rango = 0;
            }
        }
        verificar("los doubles de msws64 quedan en [0, 1)", en_rango);
    }
    {
        int acotado = 1;
        msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
        for (i = 0U; i < 10000U; ++i) {
            if (msws32_widynski_next_bounded(&r32, 7U) >= 7U) {
                acotado = 0;
            }
        }
        verificar("el valor acotado de msws32 queda en [0, 7)", acotado);
    }
    {
        int acotado = 1;
        msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                              MSWS64_WIDYNSKI_S2_PAPER);
        for (i = 0U; i < 10000U; ++i) {
            if (msws64_widynski_next_bounded(&r64, 1000000007ULL) >= 1000000007ULL) {
                acotado = 0;
            }
        }
        verificar("el valor acotado de msws64 queda dentro del rango", acotado);
    }

    printf("\n--- Uniformidad ---\n");
    {
        unsigned long long cuentas[16];
        double chi2;
        memset(cuentas, 0, sizeof(cuentas));
        msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
        for (i = 0U; i < 1000000U; ++i) {
            uint32_t v = msws32_widynski_next(&r32);
            unsigned int bin = (unsigned int)((v >> 28) & 0xFU);
            cuentas[bin]++;
        }
        chi2 = chi_cuadrado(cuentas, 16U, 1000000ULL);
        /* valor critico de 15 grados de libertad al 99%: 30.58 */
        verificar("msws32 se ve uniforme (chi2 < 30.58)", chi2 < 30.58);
        printf("        chi2 msws32 (16 bins, 1000000 muestras) = %.2f\n", chi2);
    }
    {
        unsigned long long cuentas[16];
        double chi2;
        memset(cuentas, 0, sizeof(cuentas));
        msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                              MSWS64_WIDYNSKI_S2_PAPER);
        for (i = 0U; i < 1000000U; ++i) {
            uint64_t v = msws64_widynski_next(&r64);
            unsigned int bin = (unsigned int)((v >> 60) & 0xFU);
            cuentas[bin]++;
        }
        chi2 = chi_cuadrado(cuentas, 16U, 1000000ULL);
        verificar("msws64 se ve uniforme (chi2 < 30.58)", chi2 < 30.58);
        printf("        chi2 msws64 (16 bins, 1000000 muestras) = %.2f\n", chi2);
    }
    {
        unsigned long long cuentas[16];
        double chi2;
        memset(cuentas, 0, sizeof(cuentas));
        msws64_mono_widynski_seed(&rmono, 20261120ULL);
        for (i = 0U; i < 1000000U; ++i) {
            uint64_t v = msws64_mono_widynski_next(&rmono);
            unsigned int bin = (unsigned int)((v >> 60) & 0xFU);
            cuentas[bin]++;
        }
        chi2 = chi_cuadrado(cuentas, 16U, 1000000ULL);
        verificar("la variante mono se ve uniforme (chi2 < 30.58)", chi2 < 30.58);
        printf("        chi2 mono (16 bins, 1000000 muestras) = %.2f\n", chi2);
    }

    printf("\n=================================================================\n");
    printf("  %d/%d pruebas pasaron\n", pruebas_aprobadas, pruebas_totales);
    printf("=================================================================\n");

    return (pruebas_aprobadas == pruebas_totales) ? 0 : 1;
}
