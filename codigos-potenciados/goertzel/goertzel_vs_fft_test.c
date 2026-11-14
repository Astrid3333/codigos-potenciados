/* goertzel_vs_fft_test.c - Comparacion del algoritmo de Goertzel contra una
 * FFT radix-2 sobre las mismas señales.
 *
 * Comprueba que las cuatro formulaciones (K, α, β y FFT) coinciden con la
 * magnitud analítica |X[k]| = N*A/2 de un tono cosenoidal en el bin k, y que
 * la FFT implementada coincide con la DFT directa.
 *
 * Compilar: gcc -std=c11 -O2 goertzel.c goertzel_vs_fft_test.c -lm -o prueba
 * Ejecutar: ./prueba
 */

#include "goertzel.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

/* DFT directa (referencia lenta) */
static void dft_directa(const double *x, size_t n, double *re, double *im) {
    size_t k, j;
    for (k = 0U; k < n; ++k) {
        double sr = 0.0, si = 0.0;
        for (j = 0U; j < n; ++j) {
            double ang = -2.0 * M_PI * (double)k * (double)j / (double)n;
            sr += x[j] * cos(ang);
            si += x[j] * sin(ang);
        }
        re[k] = sr;
        im[k] = si;
    }
}

int main(void) {
    const size_t N = 8192U;
    const size_t bins[3] = {1U, 1000U, 4095U};
    const double amps[3] = {1.0, 0.5, 0.25};
    const double fases[3] = {0.3, 1.1, -0.7};
    double *x;
    double *re;
    double *im;
    size_t j, i;

    printf("=================================================================\n");
    printf("  PRUEBAS - GOERTZEL CONTRA FFT RADIX-2\n");
    printf("=================================================================\n\n");

    x = (double *)malloc(N * sizeof(double));
    re = (double *)malloc(N * sizeof(double));
    im = (double *)malloc(N * sizeof(double));
    if (!x || !re || !im) {
        fprintf(stderr, "Error: no se pudo asignar memoria\n");
        free(x); free(re); free(im);
        return 1;
    }

    printf("--- La FFT radix-2 coincide con la DFT directa ---\n");
    {
        const size_t M = 64U;
        double *xr = (double *)malloc(M * sizeof(double));
        double *xi = (double *)malloc(M * sizeof(double));
        double *dr = (double *)malloc(M * sizeof(double));
        double *di = (double *)malloc(M * sizeof(double));
        double error_max = 0.0;
        if (!xr || !xi || !dr || !di) {
            fprintf(stderr, "Error: no se pudo asignar memoria\n");
            return 1;
        }
        for (j = 0U; j < M; ++j) {
            xr[j] = sin(0.3 * (double)j) + 0.5 * cos(1.1 * (double)j) +
                    0.25 * ((double)j / (double)M);
            xi[j] = 0.0;
        }
        memcpy(dr, xr, M * sizeof(double));
        memcpy(di, xi, M * sizeof(double));
        dft_directa(xr, M, dr, di);
        verificar("la FFT acepta una potencia de dos",
                  goertzel_fft(xr, xi, M) == 0);
        for (j = 0U; j < M; ++j) {
            double e = fabs(dr[j] - xr[j]) + fabs(di[j] - xi[j]);
            if (e > error_max) {
                error_max = e;
            }
        }
        printf("        error maximo FFT vs DFT directa = %.3e\n", error_max);
        verificar("la FFT coincide con la DFT directa (error < 1e-12)",
                  error_max < 1e-12);
        verificar("la FFT rechaza un n que no es potencia de dos",
                  goertzel_fft(xr, xi, 100U) == -1);
        verificar("la FFT rechaza n = 0", goertzel_fft(xr, xi, 0U) == -1);
        free(xr); free(xi); free(dr); free(di);
    }

    printf("\n--- Tonos puros: Goertzel, FFT y valor analitico ---\n");
    for (j = 0U; j < N; ++j) {
        x[j] = 0.0;
    }
    for (i = 0U; i < 3U; ++i) {
        for (j = 0U; j < N; ++j) {
            double w = 2.0 * M_PI * (double)bins[i] * (double)j / (double)N;
            x[j] += amps[i] * cos(w + fases[i]);
        }
    }
    memcpy(re, x, N * sizeof(double));
    for (j = 0U; j < N; ++j) {
        im[j] = 0.0;
    }
    verificar("la FFT calcula la señal de N = 8192", goertzel_fft(re, im, N) == 0);

    printf("\n%6s %14s %14s %14s %14s %14s\n", "bin", "analitico", "FFT",
           "Goertzel K", "Goertzel a", "Goertzel b");
    for (i = 0U; i < 3U; ++i) {
        size_t k = bins[i];
        double w = 2.0 * M_PI * (double)k / (double)N;
        double analitico = 0.5 * (double)N * amps[i];
        double mag_fft = goertzel_fft_mag(re, im, N, k);
        goertzel_cpx_t cK = goertzel_K(x, N, w);
        goertzel_cpx_t cA = goertzel_alpha(x, N, w);
        goertzel_cpx_t cB = goertzel_beta(x, N, w);
        double mag_k = goertzel_mag(&cK);
        double mag_a = goertzel_mag(&cA);
        double mag_b = goertzel_mag(&cB);
        double mayor = mag_k;
        double menor = mag_k;
        double spread;
        if (mag_fft > mayor) { mayor = mag_fft; }
        if (mag_fft < menor) { menor = mag_fft; }
        if (mag_a > mayor) { mayor = mag_a; }
        if (mag_a < menor) { menor = mag_a; }
        if (mag_b > mayor) { mayor = mag_b; }
        if (mag_b < menor) { menor = mag_b; }
        spread = mayor - menor;

        printf("%6zu %14.6f %14.6f %14.6f %14.6f %14.6f  (spread %.3e)\n",
               k, analitico, mag_fft, mag_k, mag_a, mag_b, spread);

        verificar("la FFT coincide con la magnitud analitica (rel < 1e-10)",
                  fabs(mag_fft - analitico) / analitico < 1e-10);
        verificar("Goertzel K coincide con la FFT (rel < 1e-8)",
                  fabs(mag_k - mag_fft) / mag_fft < 1e-8);
        verificar("Goertzel alpha coincide con la FFT (rel < 1e-8)",
                  fabs(mag_a - mag_fft) / mag_fft < 1e-8);
        verificar("Goertzel beta coincide con la FFT (rel < 1e-8)",
                  fabs(mag_b - mag_fft) / mag_fft < 1e-8);
        verificar("las cuatro variantes coinciden (spread rel < 1e-8)",
                  spread / mayor < 1e-8);
    }

    printf("\n--- La hibrida elige la variante correcta segun la zona ---\n");
    {
        size_t k = 1U;
        double w = 2.0 * M_PI * (double)k / (double)N;
        goertzel_cpx_t cH = goertzel_hybrid(x, N, w);
        goertzel_cpx_t cA = goertzel_alpha(x, N, w);
        double mag_h = goertzel_mag(&cH);
        double mag_a = goertzel_mag(&cA);
        double mag_fft = goertzel_fft_mag(re, im, N, k);
        verificar("la hibrida usa alpha para w < pi/2",
                  fabs(mag_h - mag_a) == 0.0);
        verificar("la hibrida coincide con la FFT (rel < 1e-8)",
                  fabs(mag_h - mag_fft) / mag_fft < 1e-8);
    }

    printf("\n--- Un tono en el bin de Nyquist ---\n");
    {
        const size_t k = N / 2U;
        const double fase = 0.4;
        double *y = (double *)malloc(N * sizeof(double));
        double *yr = (double *)malloc(N * sizeof(double));
        double *yi = (double *)malloc(N * sizeof(double));
        double w = 2.0 * M_PI * (double)k / (double)N;
        /* en Nyquist el coseno no se reparte en dos: |X[N/2]| = N*|cos(fase)| */
        double analitico = (double)N * fabs(cos(fase));
        if (!y || !yr || !yi) {
            fprintf(stderr, "Error: no se pudo asignar memoria\n");
            return 1;
        }
        for (j = 0U; j < N; ++j) {
            y[j] = cos(w * (double)j + fase);
        }
        memcpy(yr, y, N * sizeof(double));
        for (j = 0U; j < N; ++j) {
            yi[j] = 0.0;
        }
        goertzel_fft(yr, yi, N);
        verificar("la FFT del bin de Nyquist coincide con el analitico (rel < 1e-10)",
                  fabs(goertzel_fft_mag(yr, yi, N, k) - analitico) / analitico < 1e-10);
        {
            goertzel_cpx_t cB = goertzel_beta(y, N, w);
            verificar("Goertzel beta es la forma precisa cerca de pi (rel < 1e-8)",
                      fabs(goertzel_mag(&cB) - analitico) / analitico < 1e-8);
        }
        free(y); free(yr); free(yi);
    }

    free(x);
    free(re);
    free(im);

    printf("\n=================================================================\n");
    printf("  %d/%d pruebas pasaron\n", pruebas_aprobadas, pruebas_totales);
    printf("=================================================================\n");

    return (pruebas_aprobadas == pruebas_totales) ? 0 : 1;
}
