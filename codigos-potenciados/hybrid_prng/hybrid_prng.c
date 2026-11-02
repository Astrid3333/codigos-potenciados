/* hybrid_prng.c - Generador PRNG híbrido (middle-square + msws64)
 *
 * Compilar: gcc -std=c11 -O2 hybrid_prng.c -o hybrid_prng_test -lm
 * Ejecutar: ./hybrid_prng_test
 *
 * Combina middle-square de von Neumann (componente histórico, 4 dígitos)
 * con msws64 de Widynski (estado 64 bits). middle-square por sí solo está
 * roto (ciclos cortos, colapso a cero); msws64 por sí solo es un PRNG
 * decente. El híbrido XOR de ambos produce buena distribución estadística
 * sin depender exclusivamente de un solo mecanismo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include "hybrid_prng.h"

/* ---------- Estadísticas básicas ---------- */
static double mean_of(uint64_t *samples, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += (double)samples[i];
    return sum / (double)n;
}

static double chi_square_uniform(uint64_t *samples, int n, int nbins, uint64_t modulus) {
    int *counts = calloc(nbins, sizeof(int));
    double expected = (double)n / (double)nbins;
    double chi2 = 0.0;
    for (int i = 0; i < n; ++i) {
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

/* ---------- Test de ciclo para middle-square solo ---------- */
static int ms_cycles_4(void) {
    int seen[10000] = {0};
    int n_cycles = 0;
    for (uint16_t x0 = 0; x0 < 10000; ++x0) {
        uint16_t x = x0;
        int tr[10000];
        int tr_len = 0;
        while (!seen[x]) {
            seen[x] = 1;
            tr[tr_len++] = x;
            /* middle-square puro */
            uint32_t sq = (uint32_t)x * (uint32_t)x;
            uint16_t d1 = (sq / 1000) % 10;
            uint16_t d2 = (sq / 100)  % 10;
            uint16_t d3 = (sq / 10)   % 10;
            uint16_t d4 =  sq         % 10;
            x = (uint16_t)(d1 * 1000 + d2 * 100 + d3 * 10 + d4);
        }
        int found = 0;
        for (int i = 0; i < tr_len; ++i) {
            if (tr[i] == x) { found = 1; break; }
        }
        if (found) n_cycles++;
    }
    return n_cycles;
}

static int ms_zero_sink_4(void) {
    int zero_sink = 0;
    for (uint16_t x0 = 0; x0 < 10000; ++x0) {
        uint16_t x = x0;
        for (int i = 0; i < 50; ++i) {
            uint32_t sq = (uint32_t)x * (uint32_t)x;
            uint16_t d1 = (sq / 1000) % 10;
            uint16_t d2 = (sq / 100)  % 10;
            uint16_t d3 = (sq / 10)   % 10;
            uint16_t d4 =  sq         % 10;
            x = (uint16_t)(d1 * 1000 + d2 * 100 + d3 * 10 + d4);
        }
        if (x == 0) zero_sink++;
    }
    return zero_sink;
}

int main(void) {
    printf("=== hybrid_prng (middle-square + msws64) - Test ===\n\n");

    /* 1. middle-square original: referencia de falla */
    int cycles = ms_cycles_4();
    int zsink = ms_zero_sink_4();
    printf("Middle-square original (4 digitos):\n");
    printf("  Ciclos: %d\n", cycles);
    printf("  Zero sink: %d de 10000 (%.2f%%)\n", zsink, 100.0 * zsink / 10000.0);
    printf("  Estado: ROTADO como PRNG independiente\n\n");

    /* 2. hibrido: pruebas estadisticas */
    hybrid_prng_t rng;
    hybrid_prng_seed(&rng, 1234ULL);

    const int n_samples = 100000;
    const int nbins = 10;
    uint64_t *samples = malloc(n_samples * sizeof(uint64_t));
    double *doubles = malloc(n_samples * sizeof(double));

    for (int i = 0; i < n_samples; ++i) {
        samples[i] = hybrid_prng_next(&rng);
        doubles[i] = hybrid_prng_next_double(&rng);
    }

    double chi2 = chi_square_uniform(samples, n_samples, nbins, UINT64_MAX);
    double mean = mean_of(samples, n_samples);
    double mean_d = 0.0;
    for (int i = 0; i < n_samples; ++i) mean_d += doubles[i];
    mean_d /= (double)n_samples;

    printf("Hybrid PRNG (middle-square + msws64):\n");
    printf("  Semilla: 1234\n");
    printf("  Muestras: %d\n", n_samples);
    printf("  Chi^2 (%d bins): %.2f (esperado ~%d para uniformidad)\n",
           nbins, chi2, nbins - 1);
    printf("  Media uint64: %.0f (esperado ~%llu)\n",
           mean, (unsigned long long)(UINT64_MAX / 2));
    printf("  Media double: %.10f (esperado ~0.5)\n", mean_d);

    /* 3. Primeros valores */
    hybrid_prng_seed(&rng, 1234ULL);
    printf("\nPrimeros 10 valores (hibrido):\n");
    for (int i = 0; i < 10; ++i) {
        uint64_t r = hybrid_prng_next(&rng);
        double d = hybrid_prng_next_double(&rng);
        printf("  %2d: uint64=%llu, double=%.10f\n",
               i + 1, (unsigned long long)r, d);
    }

    /* 4. Uso pratico: generar IDs unicos (ejemplo tangible) */
    printf("\nUso practico: generar IDs de 64 bits (ej. IDs de transaccion):\n");
    hybrid_prng_seed(&rng, 987654321ULL);
    for (int i = 0; i < 5; ++i) {
        uint64_t id = hybrid_prng_next(&rng);
        printf("  ID %d: %016llX\n", i + 1, (unsigned long long)id);
    }

    free(samples);
    free(doubles);
    return 0;
}