/* msws64.c - Generador PRNG Widynski msws64 (2017)
 *
 * Compilar: gcc -std=c11 -O2 msws64.c -o msws64_test -lm
 * Ejecutar: ./msws64_test
 *
 * Implementa el generador de Widynski (Middle Square Weyl Sequence de 64 bits),
 * una alternativa moderna y rapida al middle-square de von Neumann.
 *
 * Comparativa incluida en el test:
 *   - middle-square original (4 digitos): ciclos cortos, colapso a 0
 *   - msws64: estado 64 bits, periodo muy largo, buena distribucion
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "msws64.h"

/* ---------- middle-square de von Neumann (4 digitos) ---------- */
static uint16_t ms_next_4(uint16_t x) {
    uint32_t sq = (uint32_t)x * (uint32_t)x;
    /* extraer 4 digitos del medio de 8 digitos con leading zeros */
    char s[12];
    snprintf(s, sizeof(s), "%08u", sq);
    uint16_t mid = (uint16_t)((s[2] - '0') * 1000 +
                               (s[3] - '0') * 100 +
                               (s[4] - '0') * 10 +
                               (s[5] - '0'));
    return mid;
}

/* ---------- test de ciclo ---------- */
static int count_cycles_4(void) {
    int seen[10000] = {0};
    int n_cycles = 0;
    for (uint16_t x0 = 0; x0 < 10000; ++x0) {
        uint16_t x = x0;
        int tr[10000];
        int tr_len = 0;
        while (!seen[x]) {
            seen[x] = 1;
            tr[tr_len++] = x;
            x = ms_next_4(x);
        }
        /* verificar si x esta en tr (ciclo) */
        int found = 0;
        for (int i = 0; i < tr_len; ++i) {
            if (tr[i] == x) { found = 1; break; }
        }
        if (found) n_cycles++;
    }
    return n_cycles;
}

static int count_zero_sink_4(void) {
    int zero_sink = 0;
    for (uint16_t x0 = 0; x0 < 10000; ++x0) {
        uint16_t x = x0;
        for (int i = 0; i < 50; ++i) x = ms_next_4(x);
        if (x == 0) zero_sink++;
    }
    return zero_sink;
}

/* ---------- chi cuadrado de uniformidad ---------- */
static double chi_square(uint64_t *samples, int n_samples, int nbins, uint64_t modulus) {
    int *counts = calloc((size_t)nbins, sizeof(int));
    if (!counts) {
        fprintf(stderr, "Error: no se pudo asignar memoria para counts\n");
        return -1.0;
    }
    double expected = (double)n_samples / (double)nbins;
    double chi2 = 0.0;
    for (int i = 0; i < n_samples; ++i) {
        int b = (int)((double)samples[i] / (double)modulus * (double)nbins);
        if (b >= nbins) b = nbins - 1;
        counts[b]++;
    }
    for (int i = 0; i < nbins; ++i) {
        double diff = (double)counts[i] - expected;
        chi2 += diff * diff / expected;
    }
    free(counts);
    return chi2;
}

int main(void) {
    printf("=== msws64 (Widynski 2017) vs middle-square de von Neumann ===\n\n");

    /* Middle-square original */
    int n_cycles = count_cycles_4();
    int zero_sink = count_zero_sink_4();
    printf("Middle-square original (4 digitos):\n");
    printf("  Ciclos: %d\n", n_cycles);
    printf("  Zero sink: %d de 10000 (%.2f%%)\n", zero_sink, 100.0 * zero_sink / 10000.0);

    /* msws64 */
    msws64_t rng;
    msws64_seed(&rng, 1234ULL);

    /* periodo de msws64 (muestreo limitado) */
    uint64_t x = rng.state;
    uint64_t seen_set[100000] = {0};
    int msws_period = 0;
    for (int i = 0; i < 100000; ++i) {
        x = msws64_next(&rng);
        int found = 0;
        for (int j = 0; j < i; ++j) {
            if (seen_set[j] == x) { found = 1; msws_period = i - j; break; }
        }
        if (found) break;
        seen_set[i] = x;
    }
    printf("\nmsws64 (estado 64 bits):\n");
    printf("  Periodo (muestra 100k): %d iteraciones sin repetir\n", msws_period > 0 ? msws_period : 100000);

    /* Chi cuadrado para msws64 */
    const int n_samples = 100000;
    const int nbins = 10;
    uint64_t *samples = malloc((size_t)n_samples * sizeof(uint64_t));
    if (!samples) {
        fprintf(stderr, "Error: no se pudo asignar memoria para samples\n");
        return 1;
    }
    msws64_seed(&rng, 1234ULL);
    for (int i = 0; i < n_samples; ++i) {
        samples[i] = msws64_next(&rng);
    }
    double chi2 = chi_square(samples, n_samples, nbins, UINT64_MAX);
    printf("  Chi^2 (%d bins, %d muestras): %.2f\n", nbins, n_samples, chi2);
    printf("  Esperado para uniformidad: ~%d (grados de libertad = %d)\n", nbins - 1, nbins - 1);

    /* Ejemplos de salida */
    printf("\nPrimeros 10 valores de msws64 (semilla 1234):\n");
    msws64_seed(&rng, 1234ULL);
    for (int i = 0; i < 10; ++i) {
        uint64_t r = msws64_next(&rng);
        double d = msws64_next_double(&rng);
        printf("  %2d: uint64=%llu, double=%.10f\n", i + 1, (unsigned long long)r, d);
    }

    free(samples);
    return 0;
}