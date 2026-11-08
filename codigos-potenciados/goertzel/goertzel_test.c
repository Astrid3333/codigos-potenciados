/* goertzel_test.c - Prueba del algoritmo de Goertzel con tres formulaciones
 *
 * Compara las tres formulaciones del algoritmo de Goertzel:
 *   - K = 2*cos(w)       (forma original, zona media)
 *   - α = 4*sin^2(w/2)   (forma para ω pequeño, cerca de 0)
 *   - β = 4*cos^2(w/2)   (espejo para w cerca de π)
 *   - híbrida: α si w < π/2, β si w >= π/2
 *
 * Para un señal de ruido gaussiano con tonos en bins específicos,
 * todas las formulaciones deberían dar aproximadamente el mismo
 * resultado (dentro de la precisión numérica).
 *
 * Compilar: gcc -std=c11 -O2 goertzel.c goertzel_test.c -lm -o goertzel_test
 * Ejecutar: ./goertzel_test
 */

#include "goertzel.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    const int N = 1 << 17;  /* 131072 muestras */
    /* srand(1) intencional: pruebas deterministas y reproducibles.
     * No se usa para claves criptográficas ni nonces. */
    srand(1);

    double *x = malloc((size_t)N * sizeof(double));
    if (!x) {
        fprintf(stderr, "Error: no se pudo asignar memoria\n");
        return 1;
    }

    /* Generar señal de ruido gaussiano con tonos en bins específicos */
    for (int i = 0; i < N; i++) {
        x[i] = 0.0;
    }

    int bins[] = {1, 10, 1000, 32768, 65535};
    double Amplitudes[] = {1.0, 0.5, 0.25, 0.5, 0.25};
    int n_bins = sizeof(bins) / sizeof(bins[0]);

    for (int i = 0; i < n_bins; i++) {
        int bin = bins[i];
        double A = Amplitudes[i];
        for (int j = 0; j < N; j++) {
            double w = 2.0 * M_PI * bin / N;
            x[j] += A * cos(w * j);
        }
    }

    printf("N = %d, ruido gaussiano con %d tonos\n\n", N, n_bins);
    printf("%7s %12s %14s %12s %12s\n", "bin", "omega", "K=2cos", "alpha", "hibrida");

    for (int i = 0; i < n_bins; i++) {
        int bin = bins[i];
        double w = 2.0 * M_PI * bin / N;

        goertzel_cpx_t cK = goertzel_K(x, (size_t)N, w);
        goertzel_cpx_t cA = goertzel_alpha(x, (size_t)N, w);
        goertzel_cpx_t cH = goertzel_hybrid(x, (size_t)N, w);

        printf("%7d %12.6f %14.6f %14.6f %14.6f\n",
               bin, w,
               goertzel_mag(&cK), goertzel_mag(&cA), goertzel_mag(&cH));
    }

    printf("\n");
    free(x);
    return 0;
}