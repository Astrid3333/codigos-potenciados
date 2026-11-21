/* msws_widynski_test.c - Pruebas del modulo msws_widynski
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra -pedantic msws_widynski_test.c -o prueba -lm
 * Ejecutar: ./prueba
 *
 * Las secuencias de referencia usadas en estas pruebas se congelaron desde tres
 * implementaciones externas INDEPENDIENTES, nunca desde este modulo:
 *   - la implementacion de referencia del autor (Bernard Widynski), version 5
 *     (mswsrngv5: msws32.h y msws64.h), GPLv3;
 *   - el crate Rust "msws" (twe4ked/msws) para msws32;
 *   - el crate Rust "weyl" (tidwall/weyl-rs, licencia ISC) para la variante
 *     mono de 64 bits.
 * Los vectores del crate "msws" coinciden bit a bit con los de la
 * implementacion del autor, de modo que son dos oraculos concordantes.
 *
 * Las semillas congeladas son todas impares, de modo que la fuerza s | 1 del
 * modulo no altera ninguna de ellas y los vectores externos son comparables
 * sin ninguna correccion adicional.
 *
 * Los vectores degenerados (s = 1 y s1 = 1, s2 = 3) se congelaron tambien
 * desde la referencia del autor: reproducen una trampa de semilla documentada
 * en el articulo, en la que el generador se queda atrapado. Este modulo los
 * reproduce con fidelidad en lugar de "corregirlos" en silencio, porque
 * cambiar el comportamiento divergiria de la referencia.
 *
 * El bloque estadistico calcula un p-valor exacto de chi-cuadrado mediante la
 * funcion gamma incompleta (mismo esquema de gser/gcf que Numerical Recipes).
 * El helper se autovalida contra valores calculados con mpmath, y los
 * detectores se prueban ademas contra generadores defectuosos conocidos para
 * comprobar que un detector que solo pasa con material bueno reproduce el
 * punto ciego.
 */

#include "msws_widynski.h"
#include "../mult_xorshift64/mult_xorshift64.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MUESTRAS_ESTADISTICAS (1ULL << 22)
#define BINS_ESTADISTICOS 256U
/* Bonferroni exacto sobre los 36 p-valores estadisticos que evalua este
 * archivo (3 variantes x 6 semillas x 2 metricas: histograma y balance de
 * bits), con alfa familiar = 0.001. */
#define UMBRAL_PVALOR (0.001 / 36.0)
#define UMBRAL_CORRELACION 0.0025

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

/* ------------------------------------------------------------------ */
/* P-valor exacto de chi-cuadrado                                       */
/* ------------------------------------------------------------------ */

static double msws_test_gammaln(double x) {
    return lgamma(x);
}

static double msws_test_gser(double a, double x) {
    double ap = a;
    double suma = 1.0 / a;
    double del = suma;
    int n;
    for (n = 0; n < 1000; ++n) {
        ap += 1.0;
        del *= x / ap;
        suma += del;
        if (fabs(del) < fabs(suma) * 3.0e-16) {
            break;
        }
    }
    return suma * exp(-x + a * log(x) - msws_test_gammaln(a));
}

static double msws_test_gcf(double a, double x) {
    double b = x + 1.0 - a;
    double c = 1.0 / 1.0e-300;
    double d = 1.0 / b;
    double h = d;
    int i;
    for (i = 1; i <= 1000; ++i) {
        double an = -i * (i - a);
        b += 2.0;
        d = an * d + b;
        if (fabs(d) < 1.0e-300) {
            d = 1.0e-300;
        }
        c = b + an / c;
        if (fabs(c) < 1.0e-300) {
            c = 1.0e-300;
        }
        d = 1.0 / d;
        {
            double del = d * c;
            h *= del;
            if (fabs(del - 1.0) < 3.0e-16) {
                break;
            }
        }
    }
    return exp(-x + a * log(x) - msws_test_gammaln(a)) * h;
}

static double msws_test_chi2_pvalor(double chi2, double df) {
    if (chi2 <= 0.0 || df <= 0.0) {
        return 1.0;
    }
    if (chi2 < df / 2.0 + 1.0) {
        return 1.0 - msws_test_gser(df / 2.0, chi2 / 2.0);
    }
    return msws_test_gcf(df / 2.0, chi2 / 2.0);
}

static double msws_test_chi_cuadrado(const unsigned long long cuentas[],
                                     unsigned int bins,
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
 * bit a bit contra el crate Rust "msws" (twe4ked/msws). */
static const uint64_t semillas_msws32[6] = {
    0xb5ad4eceda1ce2a9ULL, 0x0123456789abcdefULL, 0xfedcba9876543211ULL,
    0x9f32e1cbc5e1374bULL, 0x0f1e2d3c4b5a6979ULL, 0x91c43526df517a8bULL
};
static const uint32_t esperado_msws32[6][12] = {
    {0xb5ad4eceU, 0xdf4ee85cU, 0x1889155fU, 0xc6dcbccfU, 0x1106e0c5U, 0x473066aeU,
     0x374ac427U, 0x21e9e9bfU, 0x7294da52U, 0x212dbe1aU, 0x9c449b2aU, 0xa2b9f2fcU},
    {0x01234567U, 0x94e46287U, 0x332e9734U, 0xa1419987U, 0x5b638318U, 0x75a8b78eU,
     0xfa5936a5U, 0x120df04fU, 0x25a9fe2fU, 0x011769d6U, 0x657edb5eU, 0xb28c7cebU},
    {0xfedcba98U, 0xa1685df7U, 0xb3dd75a2U, 0x5dd81386U, 0x633df2eaU, 0x492ec9a5U,
     0x05d6091cU, 0xbcc42192U, 0xc1dfa04eU, 0x790dbfaeU, 0x66950e04U, 0x69dfc912U},
    {0x9f32e1cbU, 0x1b118920U, 0x325fcb43U, 0xea1c5833U, 0xbfca1579U, 0xf904c997U,
     0x1d30157bU, 0x8df84118U, 0xfacb5fccU, 0x71cc5669U, 0x1242a080U, 0x2f0bcf76U},
    {0x0f1e2d3cU, 0x20f2e20cU, 0xcb4f06d8U, 0x6d2ccdc0U, 0x8211ef3aU, 0x02b723c6U,
     0x98e9c6f1U, 0xb507f622U, 0x69493a4eU, 0x4d91032dU, 0x926f8bf0U, 0x5590678bU},
    {0x91c43526U, 0x9a4e3428U, 0xea8b7838U, 0xc9dc9acdU, 0x81dccbeaU, 0x8928cc5aU,
     0xe125c400U, 0x1e441951U, 0xd44f761aU, 0xadeb5a99U, 0xb8f53f01U, 0xfff28629U}
};

/* msws64 de dos carriles: implementacion de referencia del autor v5. */
static const uint64_t semillas_msws64[6][2] = {
    {0xb5ad4eceda1ce2a9ULL, 0x278c5a4d8419fe6bULL},
    {0x0123456789abcdefULL, 0xfedcba9876543211ULL},
    {0x9f32e1cbc5e1374bULL, 0x38ea2514b48de29fULL},
    {0x0f1e2d3c4b5a6979ULL, 0x7e6d5c4b3a291807ULL},
    {0x91c43526df517a8bULL, 0x0f0f0f0f0f0f0f0fULL},
    {0xdeadbeefcafebabfULL, 0x13579bdf2468ace1ULL}
};
static const uint64_t esperado_msws64[6][12] = {
    {0x31b4b0a5fd90b8e4ULL, 0xbd08dfa36824fe79ULL, 0xf895643d60a55706ULL,
     0x644e3a9a663281d9ULL, 0x642616e2d6d74b35ULL, 0x5c3ac8f51a20305cULL,
     0xd90648f19f25d755ULL, 0xb1788103945cec2fULL, 0xc9d91df5596c0239ULL,
     0x08c4143ddfa78f4cULL, 0x8adc76f7bafd82c3ULL, 0xe6cd28e8892aafc8ULL},
    {0x7777777677777777ULL, 0x5d0dfce54f3a16b8ULL, 0xbf076db0e8ba795cULL,
     0x1453db4fd37589caULL, 0xc421a761b3c65836ULL, 0x1e54ddc4b896ac7fULL,
     0x9634d475f878fd51ULL, 0x17c97bd727065443ULL, 0x214547f2fd7bd586ULL,
     0xe698e5b8f428e859ULL, 0xfdb19ba1a002162dULL, 0xbfd2b437255054aaULL},
    {0x2bbf0354fd0b125fULL, 0xec1b07ee22e00aeeULL, 0x287821ddf99c3463ULL,
     0x291518b3eb272d33ULL, 0x84a31a46edb4122aULL, 0x610c8389ba31ad52ULL,
     0xb05e28e1585de7d5ULL, 0xead0f2e9d87a03a0ULL, 0x71a0bdf4e7f7e6aaULL,
     0x9dee1f6e0229c1ffULL, 0xa59a4156fedcc5caULL, 0x10b7100295c3e00eULL},
    {0x3537353b35373532ULL, 0x2184cc0b725b5340ULL, 0xa1d407c1f3ef13c7ULL,
     0xda843becba5c65cfULL, 0x9df4dae65567d705ULL, 0x7b570dac8412e514ULL,
     0x1dd4f38477a0ebf0ULL, 0x42504d63a27420cfULL, 0x737282adaf88bf88ULL,
     0xc82cb042175a651aULL, 0x69134cadd14f8402ULL, 0xb5a35c535988caa6ULL},
    {0x9ecb3a29d05e7584ULL, 0x3e8cd4d71b91de3cULL, 0x3fd92b6953e1244eULL,
     0xc6056b106995b2d0ULL, 0x31df7eb1b4f8f936ULL, 0x6092e564b08dfea8ULL,
     0x9a3b182d7c740118ULL, 0xb464ead8525b5763ULL, 0xab03813ad29fc69bULL,
     0xbcdaf8cea928f7c2ULL, 0x5c1711a4f569c7d7ULL, 0xe7810e147727a1d3ULL},
    {0xfac5120ed9a92160ULL, 0xfd4c599089a3e445ULL, 0xc47827185e762e59ULL,
     0x2e3a1aeda83c3ffcULL, 0xaf6ea85ef4360f7eULL, 0x2dcac848b394fef5ULL,
     0x9d67511983618de3ULL, 0x04d1b39123ad3070ULL, 0x1f2f7d436d2e97f2ULL,
     0x16df94c6e117554fULL, 0xf1a0dc6c8d6f7286ULL, 0xf7b630ce5c07a0b0ULL}
};

/* Variante mono de 64 bits: crate Rust "weyl" (tidwall/weyl-rs, ISC).
 * Semilla -> s = (semilla << 1) + 0xb5ad4eceda1ce2a9. */
static const uint64_t semillas_weyl[6] = {
    0ULL, 1ULL, 2ULL, 12345ULL, 20261120ULL, 987654321ULL
};
static const uint64_t esperado_weyl[6][4] = {
    {0xda1ce2a9b5ad4eceULL, 0x9ae7f316df4ee85cULL, 0x414d890b1889155fULL,
     0x19ea43e5c6dcbccfULL},
    {0xda1ce2abb5ad4eceULL, 0x9ae7f31ab6042394ULL, 0x1fe87591bbec0124ULL,
     0x1ad4d7bcc44c1140ULL},
    {0xda1ce2adb5ad4eceULL, 0x9ae7f31e8cb95eccULL, 0xd4491a97d7f4b225ULL,
     0xa0f3040de4d5d3ffULL},
    {0xda1d431bb5ad4eceULL, 0x9ae8b3faa8c397d4ULL, 0x755b90e1f4e03b33ULL,
     0x4765989529f4e022ULL},
    {0xdc8734a9b5ad4eceULL, 0x9fbc97166062e05cULL, 0x20a6bf0b4a1d52b2ULL,
     0x31275668caece9b2ULL},
    {0x4fd9b40bb5ad4ecfULL, 0xf1bc33779cef022cULL, 0x9db9d3b1aaeda320ULL,
     0x3098942c649806bbULL}
};

/* Trampa de semilla de la referencia: s = 1 deja msws32 en cero, y
 * s1 = 1 con s2 = 3 deja msws64 en un patron determinista trivial. */
static const uint32_t esperado_msws32_degen[4] = {
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U
};
static const uint64_t esperado_msws64_degen[4] = {
    0x0000000300000001ULL, 0x0000000600000002ULL,
    0x0000000900000003ULL, 0x0000000c00000004ULL
};

/* Puntos de calibracion del helper de p-valor, calculados con mpmath 1.3.0
 * (gammainc regularizada). Sirven para autovalidar la implementacion de la
 * gamma incompleta incluida en este archivo. */
static const struct {
    double chi2;
    double df;
    double pvalor;
} tabla_calibracion_pvalor[] = {
    {310.4574, 255.0, 0.01},
    {330.5197, 255.0, 0.001},
    {293.2478, 255.0, 0.05},
    {30.5779, 15.0, 0.01},
    {24.9958, 15.0, 0.05},
    {37.6973, 15.0, 0.001},
    {92.0100, 63.0, 0.01},
    {166.9874, 127.0, 0.01},
    {104.7163, 64.0, 0.001},
    {62.4872, 32.0, 0.001}
};

/* ------------------------------------------------------------------ */
/* Detector estadistico reutilizable                                   */
/* ------------------------------------------------------------------ */

typedef uint64_t (*msws_stream_fn)(void *contexto);

typedef struct {
    unsigned long long cuentas[BINS_ESTADISTICOS];
    unsigned long long bits[64];
    unsigned int bits_salida;
    double suma_x;
    double suma_y;
    double suma_xy;
    double suma_xx;
    double suma_yy;
    unsigned long long n;
    uint64_t anterior;
    int tiene_anterior;
} msws_medidor_t;

static void msws_medidor_iniciar(msws_medidor_t *m, unsigned int bits_salida) {
    memset(m, 0, sizeof(*m));
    m->bits_salida = bits_salida;
}

static void msws_medidor_anadir(msws_medidor_t *m, uint64_t v) {
    double y = (double)v / 18446744073709551616.0;
    unsigned int b;
    m->cuentas[(unsigned int)((v >> (m->bits_salida - 8U)) & 0xFFULL)]++;
    for (b = 0U; b < m->bits_salida; ++b) {
        m->bits[b] += (v >> b) & 1ULL;
    }
    if (m->tiene_anterior) {
        double x = (double)m->anterior / 18446744073709551616.0;
        m->suma_x += x;
        m->suma_y += y;
        m->suma_xy += x * y;
        m->suma_xx += x * x;
        m->suma_yy += y * y;
    }
    m->anterior = v;
    m->tiene_anterior = 1;
    m->n++;
}

static double msws_medidor_chi2_histograma(const msws_medidor_t *m) {
    return msws_test_chi_cuadrado(m->cuentas, BINS_ESTADISTICOS, m->n);
}

static double msws_medidor_chi2_bits(const msws_medidor_t *m) {
    double esperado = (double)m->n / 2.0;
    double chi2 = 0.0;
    unsigned int i;
    for (i = 0U; i < m->bits_salida; ++i) {
        double d = (double)m->bits[i] - esperado;
        chi2 += d * d / esperado;
    }
    return chi2;
}

static double msws_medidor_correlacion_lag1(const msws_medidor_t *m) {
    double n = (double)m->n;
    double cov = m->suma_xy - m->suma_x * m->suma_y / n;
    double vx = m->suma_xx - m->suma_x * m->suma_x / n;
    double vy = m->suma_yy - m->suma_y * m->suma_y / n;
    if (vx <= 0.0 || vy <= 0.0) {
        return 0.0;
    }
    return cov / sqrt(vx * vy);
}

static void msws_medidor_recolectar(msws_medidor_t *m, msws_stream_fn flujo,
                                    void *contexto, unsigned long long muestras,
                                    unsigned int bits_salida) {
    unsigned long long i;
    msws_medidor_iniciar(m, bits_salida);
    for (i = 0ULL; i < muestras; ++i) {
        msws_medidor_anadir(m, flujo(contexto));
    }
}

/* ------------------------------------------------------------------ */
/* Flujos de prueba                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    msws32_widynski_t r;
} contexto_msws32_t;

static uint64_t flujo_msws32(void *contexto) {
    return (uint64_t)msws32_widynski_next(&((contexto_msws32_t *)contexto)->r);
}

typedef struct {
    msws64_widynski_t r;
} contexto_msws64_t;

static uint64_t flujo_msws64(void *contexto) {
    return msws64_widynski_next(&((contexto_msws64_t *)contexto)->r);
}

typedef struct {
    msws64_mono_widynski_t r;
} contexto_mono_t;

static uint64_t flujo_mono(void *contexto) {
    return msws64_mono_widynski_next(&((contexto_mono_t *)contexto)->r);
}

/* Generador defectuoso 1: LCG RANDU (mod 2^31, multiplicador 65539). Su byte
 * mas alto no tiene periodo completo, por lo que el detector de balance de
 * bits debe rechazarlo aunque el histograma del byte alto parezca uniforme. */
typedef struct {
    uint32_t x;
} contexto_randu_t;

static uint64_t flujo_randu(void *contexto) {
    contexto_randu_t *c = (contexto_randu_t *)contexto;
    c->x = (uint32_t)((uint64_t)c->x * 65539ULL + 12345ULL) & 0x7FFFFFFFU;
    return (uint64_t)c->x;
}

/* Generador defectuoso 2: LCG clasico (mod 2^32, multiplicador 1103515245).
 * Su byte bajo tiene periodo 256, de modo que la correlacion de retardo 1
 * es fuerte aunque el balance de bits del byte siga pareciendo correcto. */
typedef struct {
    uint32_t x;
} contexto_lcg_byte_t;

static uint64_t flujo_lcg_byte_bajo(void *contexto) {
    contexto_lcg_byte_t *c = (contexto_lcg_byte_t *)contexto;
    c->x = (uint32_t)((uint64_t)c->x * 1103515245ULL + 12345ULL);
    return (uint64_t)(c->x & 0xFFULL);
}

/* Generador defectuoso 3: flujo con el byte alto restringido a 4 valores.
 * El histograma de 256 bins debe rechazarlo de forma directa. */
static uint64_t flujo_byte_alto_restringido(void *contexto) {
    uint32_t *x = (uint32_t *)contexto;
    *x = (uint32_t)((uint64_t)(*x) * 1664525ULL + 1013904223ULL);
    return (uint64_t)((*x & 0x0FU)) << 56;
}

/* ------------------------------------------------------------------ */
/* Programa principal                                                   */
/* ------------------------------------------------------------------ */

int main(void) {
    msws32_widynski_t r32;
    msws64_widynski_t r64;
    msws64_mono_widynski_t rmono;
    size_t i;
    unsigned int k;

    printf("=================================================================\n");
    printf("  PRUEBAS - MSWS (MIDDLE SQUARE WEYL SEQUENCE) DE WIDYNSKI\n");
    printf("=================================================================\n\n");

    printf("--- Autovalidador del p-valor de chi-cuadrado (referencia mpmath) ---\n");
    {
        int coincide = 1;
        unsigned int c;
        for (c = 0U; c < sizeof(tabla_calibracion_pvalor) /
                        sizeof(tabla_calibracion_pvalor[0]); ++c) {
            double p = msws_test_chi2_pvalor(tabla_calibracion_pvalor[c].chi2,
                                             tabla_calibracion_pvalor[c].df);
            if (fabs(p - tabla_calibracion_pvalor[c].pvalor) > 1.0e-6) {
                coincide = 0;
                printf("        chi2=%.4f df=%.0f esperado p=%.6f obtenido p=%.8f\n",
                       tabla_calibracion_pvalor[c].chi2,
                       tabla_calibracion_pvalor[c].df,
                       tabla_calibracion_pvalor[c].pvalor, p);
            }
        }
        verificar("el helper de p-valor reproduce los 10 puntos de mpmath",
                  coincide);
    }

    printf("\n--- msws32: 32 bits (referencia externa del autor + crate Rust) ---\n");
    {
        int coincide = 1;
        unsigned int fallidas = 0U;
        for (k = 0U; k < 6U; ++k) {
            msws32_widynski_seed(&r32, semillas_msws32[k]);
            for (i = 0U; i < 12U; ++i) {
                uint32_t v = msws32_widynski_next(&r32);
                if (v != esperado_msws32[k][i]) {
                    coincide = 0;
                    fallidas++;
                    printf("        semilla %016llx salida %u esperada %08x, obtenida %08x\n",
                           (unsigned long long)semillas_msws32[k], (unsigned)i,
                           esperado_msws32[k][i], v);
                }
            }
        }
        verificar("los 72 vectores de msws32 (6 semillas x 12) coinciden con las fuentes externas",
                  coincide);
        printf("        %u discrepancias sobre 72 comparaciones\n", fallidas);
    }

    printf("\n--- msws64: dos carriles, 64 bits (referencia externa del autor) ---\n");
    {
        int coincide = 1;
        unsigned int fallidas = 0U;
        for (k = 0U; k < 6U; ++k) {
            msws64_widynski_seed(&r64, semillas_msws64[k][0], semillas_msws64[k][1]);
            for (i = 0U; i < 12U; ++i) {
                uint64_t v = msws64_widynski_next(&r64);
                if (v != esperado_msws64[k][i]) {
                    coincide = 0;
                    fallidas++;
                    printf("        par %u salida %u esperada %016llx, obtenida %016llx\n",
                           k, (unsigned)i, (unsigned long long)esperado_msws64[k][i],
                           (unsigned long long)v);
                }
            }
        }
        verificar("los 72 vectores de msws64 (6 pares x 12) coinciden con la referencia externa",
                  coincide);
        printf("        %u discrepancias sobre 72 comparaciones\n", fallidas);
    }

    printf("\n--- Variante mono de 64 bits (crate Rust weyl-rs) ---\n");
    {
        int coincide = 1;
        unsigned int fallidas = 0U;
        for (k = 0U; k < 6U; ++k) {
            msws64_mono_widynski_seed(&rmono, semillas_weyl[k]);
            for (i = 0U; i < 4U; ++i) {
                uint64_t v = msws64_mono_widynski_next(&rmono);
                if (v != esperado_weyl[k][i]) {
                    coincide = 0;
                    fallidas++;
                    printf("        semilla %llu salida %u esperada %016llx, obtenida %016llx\n",
                           (unsigned long long)semillas_weyl[k], (unsigned)i,
                           (unsigned long long)esperado_weyl[k][i],
                           (unsigned long long)v);
                }
            }
        }
        verificar("los 24 vectores de la variante mono (6 semillas x 4) coinciden con weyl-rs",
                  coincide);
        printf("        %u discrepancias sobre 24 comparaciones\n", fallidas);
    }

    printf("\n--- Fidelidad con las semillas degeneradas de la referencia ---\n");
    {
        int coincide = 1;
        msws32_widynski_seed(&r32, 1ULL);
        for (i = 0U; i < 4U; ++i) {
            if (msws32_widynski_next(&r32) != esperado_msws32_degen[i]) {
                coincide = 0;
            }
        }
        verificar("msws32 con s = 1 reproduce la trampa de la referencia (todo ceros)",
                  coincide);
    }
    {
        int coincide = 1;
        msws64_widynski_seed(&r64, 1ULL, 3ULL);
        for (i = 0U; i < 4U; ++i) {
            if (msws64_widynski_next(&r64) != esperado_msws64_degen[i]) {
                coincide = 0;
            }
        }
        verificar("msws64 con s1 = 1, s2 = 3 reproduce la trampa de la referencia",
                  coincide);
    }

    printf("\n--- Ejemplo del articulo (seccion 3) ---\n");
    {
        msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
        r32.x = 0xE3296D171EC4A36FULL;
        r32.w = 0ULL - r32.s;
        verificar_entero("el ejemplo del articulo produce 31C2914A",
                         (unsigned long long)msws32_widynski_next(&r32),
                         0x31C2914AULL);
    }

    printf("\n--- Consistencia interna ---\n");
    {
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
        msws32_widynski_t a;
        msws32_widynski_t b;
        msws32_widynski_t c;
        int distintos = 1;
        uint32_t va[4];
        uint32_t vb[4];
        uint32_t vc[4];
        unsigned int j;

        msws32_widynski_seed(&a, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_seed(&b, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_seed(&c, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_jump_ahead(&b, 1ULL);
        msws32_widynski_jump_ahead(&c, 2ULL);

        for (j = 0U; j < 4U; ++j) {
            va[j] = msws32_widynski_next(&a);
            vb[j] = msws32_widynski_next(&b);
            vc[j] = msws32_widynski_next(&c);
            if (va[j] == vb[j] || va[j] == vc[j] || vb[j] == vc[j]) {
                distintos = 0;
            }
        }
        verificar("los flujos con saltos 0, 1 y 2 son distintos", distintos);
    }
    {
        msws32_widynski_t a;
        int variety = 0;
        unsigned int j;
        uint32_t v0;
        msws32_widynski_seed(&a, MSWS32_WIDYNSKI_S_PAPER);
        msws32_widynski_jump_ahead(&a, 1000000000ULL);
        v0 = msws32_widynski_next(&a);
        for (j = 0U; j < 1000U; ++j) {
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

    printf("\n--- Uniformidad, balance de bits y correlacion (p-valor exacto) ---\n");
    printf("    %llu muestras por flujo, %u bins, umbral p > %.4f\n",
           MUESTRAS_ESTADISTICAS, BINS_ESTADISTICOS, UMBRAL_PVALOR);
    {
        int hist_ok = 1;
        int bits_ok = 1;
        int corr_ok = 1;
        double p_min_hist = 1.0;
        double p_min_bits = 1.0;
        double r_max = 0.0;

        for (k = 0U; k < 6U; ++k) {
            contexto_msws32_t c32;
            msws_medidor_t m;
            double chi2h;
            double chi2b;
            double p;
            double r;

            msws32_widynski_seed(&c32.r, semillas_msws32[k]);
            msws_medidor_recolectar(&m, flujo_msws32, &c32,
                                    MUESTRAS_ESTADISTICAS, 32U);

            chi2h = msws_medidor_chi2_histograma(&m);
            p = msws_test_chi2_pvalor(chi2h, (double)(BINS_ESTADISTICOS - 1U));
            if (p < p_min_hist) {
                p_min_hist = p;
            }
            if (p <= UMBRAL_PVALOR) {
                hist_ok = 0;
                printf("        msws32 semilla %u: histograma chi2=%.2f p=%.3e\n",
                       k, chi2h, p);
            }

            chi2b = msws_medidor_chi2_bits(&m);
            p = msws_test_chi2_pvalor(chi2b, 32.0);
            if (p < p_min_bits) {
                p_min_bits = p;
            }
            if (p <= UMBRAL_PVALOR) {
                bits_ok = 0;
                printf("        msws32 semilla %u: balance de bits chi2=%.2f p=%.3e\n",
                       k, chi2b, p);
            }

            r = msws_medidor_correlacion_lag1(&m);
            if (fabs(r) > fabs(r_max)) {
                r_max = r;
            }
            if (fabs(r) > UMBRAL_CORRELACION) {
                corr_ok = 0;
                printf("        msws32 semilla %u: correlacion lag-1 r=%+.6f\n", k, r);
            }
        }

        verificar("msws32: histograma de 256 bins uniforme en las 6 semillas",
                  hist_ok);
        verificar("msws32: balance de 32 posiciones sin sesgo en las 6 semillas",
                  bits_ok);
        verificar("msws32: correlacion lag-1 dentro del umbral en las 6 semillas",
                  corr_ok);
        printf("        p minimo histograma %.3e, p minimo balance %.3e, |r| maximo %.6f\n",
               p_min_hist, p_min_bits, fabs(r_max));
    }
    {
        int hist_ok = 1;
        int bits_ok = 1;
        int corr_ok = 1;
        double p_min_hist = 1.0;
        double p_min_bits = 1.0;
        double r_max = 0.0;

        for (k = 0U; k < 6U; ++k) {
            contexto_msws64_t c64;
            msws_medidor_t m;
            double chi2h;
            double chi2b;
            double p;
            double r;

            msws64_widynski_seed(&c64.r, semillas_msws64[k][0], semillas_msws64[k][1]);
            msws_medidor_recolectar(&m, flujo_msws64, &c64,
                                    MUESTRAS_ESTADISTICAS, 64U);

            chi2h = msws_medidor_chi2_histograma(&m);
            p = msws_test_chi2_pvalor(chi2h, (double)(BINS_ESTADISTICOS - 1U));
            if (p < p_min_hist) {
                p_min_hist = p;
            }
            if (p <= UMBRAL_PVALOR) {
                hist_ok = 0;
                printf("        msws64 par %u: histograma chi2=%.2f p=%.3e\n",
                       k, chi2h, p);
            }

            chi2b = msws_medidor_chi2_bits(&m);
            p = msws_test_chi2_pvalor(chi2b, 64.0);
            if (p < p_min_bits) {
                p_min_bits = p;
            }
            if (p <= UMBRAL_PVALOR) {
                bits_ok = 0;
                printf("        msws64 par %u: balance de bits chi2=%.2f p=%.3e\n",
                       k, chi2b, p);
            }

            r = msws_medidor_correlacion_lag1(&m);
            if (fabs(r) > fabs(r_max)) {
                r_max = r;
            }
            if (fabs(r) > UMBRAL_CORRELACION) {
                corr_ok = 0;
                printf("        msws64 par %u: correlacion lag-1 r=%+.6f\n", k, r);
            }
        }

        verificar("msws64: histograma de 256 bins uniforme en los 6 pares",
                  hist_ok);
        verificar("msws64: balance de 64 posiciones sin sesgo en los 6 pares",
                  bits_ok);
        verificar("msws64: correlacion lag-1 dentro del umbral en los 6 pares",
                  corr_ok);
        printf("        p minimo histograma %.3e, p minimo balance %.3e, |r| maximo %.6f\n",
               p_min_hist, p_min_bits, fabs(r_max));
    }
    {
        int hist_ok = 1;
        int bits_ok = 1;
        int corr_ok = 1;
        double p_min_hist = 1.0;
        double p_min_bits = 1.0;
        double r_max = 0.0;

        for (k = 0U; k < 6U; ++k) {
            contexto_mono_t cm;
            msws_medidor_t m;
            double chi2h;
            double chi2b;
            double p;
            double r;

            msws64_mono_widynski_seed(&cm.r, semillas_weyl[k]);
            msws_medidor_recolectar(&m, flujo_mono, &cm,
                                    MUESTRAS_ESTADISTICAS, 64U);

            chi2h = msws_medidor_chi2_histograma(&m);
            p = msws_test_chi2_pvalor(chi2h, (double)(BINS_ESTADISTICOS - 1U));
            if (p < p_min_hist) {
                p_min_hist = p;
            }
            if (p <= UMBRAL_PVALOR) {
                hist_ok = 0;
                printf("        mono semilla %u: histograma chi2=%.2f p=%.3e\n",
                       k, chi2h, p);
            }

            chi2b = msws_medidor_chi2_bits(&m);
            p = msws_test_chi2_pvalor(chi2b, 64.0);
            if (p < p_min_bits) {
                p_min_bits = p;
            }
            if (p <= UMBRAL_PVALOR) {
                bits_ok = 0;
                printf("        mono semilla %u: balance de bits chi2=%.2f p=%.3e\n",
                       k, chi2b, p);
            }

            r = msws_medidor_correlacion_lag1(&m);
            if (fabs(r) > fabs(r_max)) {
                r_max = r;
            }
            if (fabs(r) > UMBRAL_CORRELACION) {
                corr_ok = 0;
                printf("        mono semilla %u: correlacion lag-1 r=%+.6f\n", k, r);
            }
        }

        verificar("mono: histograma de 256 bins uniforme en las 6 semillas",
                  hist_ok);
        verificar("mono: balance de 64 posiciones sin sesgo en las 6 semillas",
                  bits_ok);
        verificar("mono: correlacion lag-1 dentro del umbral en las 6 semillas",
                  corr_ok);
        printf("        p minimo histograma %.3e, p minimo balance %.3e, |r| maximo %.6f\n",
               p_min_hist, p_min_bits, fabs(r_max));
    }

    printf("\n--- Controles negativos: los detectores deben rechazar material defectuoso ---\n");
    {
        contexto_randu_t rc;
        msws_medidor_t m;
        double chi2b;
        double p;

        rc.x = 1U;
        msws_medidor_recolectar(&m, flujo_randu, &rc, MUESTRAS_ESTADISTICAS, 31U);
        chi2b = msws_medidor_chi2_bits(&m);
        p = msws_test_chi2_pvalor(chi2b, 31.0);
        printf("        RANDU mod 2^31: balance de bits chi2=%.2f p=%.3e\n",
               chi2b, p);
        verificar("el detector de balance de bits rechaza a RANDU", p <= UMBRAL_PVALOR);
    }
    {
        contexto_lcg_byte_t cl;
        msws_medidor_t m;
        double r;

        cl.x = 1U;
        msws_medidor_recolectar(&m, flujo_lcg_byte_bajo, &cl,
                                MUESTRAS_ESTADISTICAS, 8U);
        r = msws_medidor_correlacion_lag1(&m);
        printf("        LCG mod 2^32 byte bajo: correlacion lag-1 r=%+.6f\n", r);
        verificar("el detector de correlacion rechaza al LCG de byte bajo",
                  fabs(r) > UMBRAL_CORRELACION);
    }
    {
        uint32_t x = 1U;
        msws_medidor_t m;
        double chi2h;
        double p;

        msws_medidor_recolectar(&m, flujo_byte_alto_restringido, &x,
                                MUESTRAS_ESTADISTICAS, 64U);
        chi2h = msws_medidor_chi2_histograma(&m);
        p = msws_test_chi2_pvalor(chi2h, (double)(BINS_ESTADISTICOS - 1U));
        printf("        byte alto restringido: histograma chi2=%.2f p=%.3e\n",
               chi2h, p);
        verificar("el detector de histograma rechaza al flujo de byte alto restringido",
                  p <= UMBRAL_PVALOR);
    }

    printf("\n=================================================================\n");
    printf("  %d/%d pruebas pasaron\n", pruebas_aprobadas, pruebas_totales);
    printf("=================================================================\n");

    return (pruebas_aprobadas == pruebas_totales) ? 0 : 1;
}
