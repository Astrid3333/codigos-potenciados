/* lcg_lehmer_test.c - Test completo de LCG de Lehmer
 *
 * Compilar: gcc -std=c11 -O2 lcg_lehmer.c lcg_lehmer_test.c -o lcg_lehmer_test -lm
 * Ejecutar: ./lcg_lehmer_test
 */

#include "lcg_lehmer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

/* Multiplicadores históricos */
#define RANDU_A  65539ULL   /* 2^16 + 3 */
#define RANDU_M  (1ULL << 31)
#define MINSTD_A 16807ULL   /* 7^5 */
#define MINSTD_M (1ULL << 31) - 1
#define BORLAND_A 22695477ULL
#define BORLAND_C 1ULL
#define BORLAND_M (1ULL << 32)

static void print_spectral(lcg_spectral_result_t *r, const char *name) {
    printf("  %-20s a=%" PRIu64 " m=%" PRIu64 " dim=%u\n", name, r->a, r->m, r->dim);
    printf("    distancia_min = %.6e\n", r->min_distance);
    printf("    num_planes    = %d\n", r->num_planes);
    printf("    spectral_score= %.4f (1.0 = ideal)\n", r->spectral_score);
    printf("    es_malo       = %s\n", r->is_bad ? "SÍ" : "no");
}

int main(void) {
    printf("=================================================================\n");
    printf("  ANÁLISIS ESPECTRAL DE LCG DE LEHMER (1949)\n");
    printf("=================================================================\n\n");

    /* ---------- 1. RANDU ---------- */
    printf("1. RANDU (a=65539=2^16+3, m=2^31)\n");
    printf("   Multiplicador histórico defectuoso\n\n");
    lcg_spectral_result_t r2 = lcg_spectral_analyze(RANDU_A, 0, RANDU_M, 2);
    lcg_spectral_result_t r3 = lcg_spectral_analyze(RANDU_A, 0, RANDU_M, 3);
    lcg_spectral_result_t r4 = lcg_spectral_analyze(RANDU_A, 0, RANDU_M, 4);
    print_spectral(&r2, "k=2");
    print_spectral(&r3, "k=3");
    print_spectral(&r4, "k=4");

    printf("   Planos en 3D (test de red Marsaglia): %d\n",
           lcg_plane_count_3d(RANDU_A, 0, RANDU_M));
    printf("   Esperado: ~15 planos (RANDU es famoso por esto)\n\n");

    /* ---------- 2. MINSTD (Lehmer original) ---------- */
    printf("2. MINSTD (a=16807=7^5, m=2^31-1, primo de Mersenne)\n");
    printf("   Generador recomendado por Park & Miller (1988)\n\n");
    lcg_spectral_result_t m2 = lcg_spectral_analyze(MINSTD_A, 0, MINSTD_M, 2);
    lcg_spectral_result_t m3 = lcg_spectral_analyze(MINSTD_A, 0, MINSTD_M, 3);
    print_spectral(&m2, "k=2");
    print_spectral(&m3, "k=3");
    printf("\n");

    /* ---------- 3. Prueba de período ---------- */
    printf("3. PRUEBA DE PERÍODO\n");
    lcg_period_result_t p_randu = lcg_period_test(RANDU_A, 0, RANDU_M, 1, 10000000);
    printf("   RANDU (x0=1): período=%" PRIu64 ", full_period=%d, zero_sink=%d\n",
           p_randu.period, p_randu.full_period, p_randu.zero_sink);

    lcg_period_result_t p_minstd = lcg_period_test(MINSTD_A, 0, MINSTD_M, 1, 10000000);
    printf("   MINSTD (x0=1): período=%" PRIu64 ", full_period=%d, zero_sink=%d\n",
           p_minstd.period, p_minstd.full_period, p_minstd.zero_sink);
    printf("\n");

    /* ---------- 4. Híbrido LCG + msws64 ---------- */
    printf("4. HÍBRIDO LCG + msws64\n");
    hybrid_lcg_msws_t h;
    hybrid_lcg_msws_seed(&h, 1234ULL);
    printf("   Primeros 10 valores híbridos:\n");
    for (int i = 0; i < 10; i++) {
        printf("     %2d: %llu\n", i + 1,
               (unsigned long long)hybrid_lcg_msws_next(&h));
    }
    printf("\n");

    /* ---------- 5. TCR-LCG ---------- */
    printf("5. TCR-LCG (Teorema Chino del Resto)\n");
    printf("   p1=9973, p2=9967, M=99,400,891\n");
    tcr_lcg_t t;
    tcr_lcg_seed(&t, 1234ULL, 9973, 9967, 65539ULL);
    printf("   Primeros 10 valores TCR-LCG:\n");
    for (int i = 0; i < 10; i++) {
        printf("     %2d: %llu\n", i + 1,
               (unsigned long long)tcr_lcg_next(&t));
    }
    printf("\n");

    /* ---------- 6. Resumen ---------- */
    printf("=================================================================\n");
    printf("  RESUMEN\n");
    printf("=================================================================\n");
    printf("  RANDU (a=65539): espectral_score k=3 = %.4f  → %s\n",
           r3.spectral_score, r3.is_bad ? "MALO (planos en 3D)" : "aceptable");
    printf("  MINSTD (a=16807): espectral_score k=3 = %.4f  → %s\n",
           m3.spectral_score, m3.is_bad ? "MALO" : "aceptable");
    printf("\n");
    printf("  Conclusión: RANDU falla el test espectral en 3D (solo ~15 planos).\n");
    printf("  MINSTD es mejor pero sigue siendo LCG lineal.\n");
    printf("  Para uso real: usar msws64 o híbrido LCG+msws64.\n");

    return 0;
}