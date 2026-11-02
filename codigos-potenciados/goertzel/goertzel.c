/* goertzel.c - Implementaciones del algoritmo de Goertzel y benchmark
 *
 * Compilar: gcc -std=c11 -O2 goertzel.c -o goertzel_bench -lm
 * Ejecutar: ./goertzel_bench
 */

#define _POSIX_C_SOURCE 199309L
#include "goertzel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- Goertzel K ---------- */
goertzel_cpx_t goertzel_K(const double *x, size_t n, double w) {
    double K = 2.0 * cos(w);
    double s1 = 0.0, s2 = 0.0, s;
    for (size_t i = 0; i < n; ++i) {
        s = x[i] + K * s1 - s2;
        s2 = s1;
        s1 = s;
    }
    s = K * s1 - s2;                     /* x[n] = 0 */
    double cw = cos(w), sw = sin(w);
    return (goertzel_cpx_t){ s - cw * s1, sw * s1 };
    /* y = s - exp(-1i*w)*s1 = s - (cos w - i sin w)*s1
     * re = s - cos(w)*s1, im = sin(w)*s1 */
}

/* ---------- Goertzel alpha ---------- */
goertzel_cpx_t goertzel_alpha(const double *x, size_t n, double w) {
    double al = 4.0 * sin(w / 2.0) * sin(w / 2.0);
    double s = 0.0, d = 0.0;
    for (size_t i = 0; i < n; ++i) {
        d = d + x[i] - al * s;
        s = s + d;
    }
    d = d - al * s;
    s = s + d;                           /* x[n] = 0 */
    double s1 = s - d;
    double cw = cos(w), sw = sin(w);
    return (goertzel_cpx_t){ s - cw * s1, sw * s1 };
}

/* ---------- Goertzel beta (espejo para w > pi/2) ---------- */
goertzel_cpx_t goertzel_beta(const double *x, size_t n, double w) {
    double be = 4.0 * cos(w / 2.0) * cos(w / 2.0);
    double s = 0.0, d = 0.0;
    for (size_t i = 0; i < n; ++i) {
        d = x[i] + be * s - d;
        s = d - s;
    }
    d = be * s - d;
    s = d - s;                           /* x[n] = 0 */
    double s1 = d - s;
    double cw = cos(w), sw = sin(w);
    return (goertzel_cpx_t){ s - cw * s1, sw * s1 };
}

/* ---------- Goertzel hybrida ---------- */
goertzel_cpx_t goertzel_hybrid(const double *x, size_t n, double w) {
    if (w < M_PI / 2.0)
        return goertzel_alpha(x, n, w);
    else
        return goertzel_beta(x, n, w);
}

/* ---------- Magnitud ---------- */
double goertzel_mag(const goertzel_cpx_t *c) {
    return sqrt(c->re * c->re + c->im * c->im);
}

/* ---------- FFT radix-2 simple ---------- */
static void fft_recursive(goertzel_cpx_t *a, size_t n) {
    if (n <= 1) return;
    goertzel_cpx_t *even = malloc(n / 2 * sizeof(goertzel_cpx_t));
    goertzel_cpx_t *odd = malloc(n / 2 * sizeof(goertzel_cpx_t));
    for (size_t i = 0; i < n / 2; ++i) {
        even[i] = a[2 * i];
        odd[i] = a[2 * i + 1];
    }
    fft_recursive(even, n / 2);
    fft_recursive(odd, n / 2);
    for (size_t k = 0; k < n / 2; ++k) {
        double ang = -2.0 * M_PI * k / (double)n;
        goertzel_cpx_t t;
        t.re = odd[k].re * cos(ang) - odd[k].im * sin(ang);
        t.im = odd[k].re * sin(ang) + odd[k].im * cos(ang);
        a[k].re = even[k].re + t.re;
        a[k].im = even[k].im + t.im;
        a[k + n / 2].re = even[k].re - t.re;
        a[k + n / 2].im = even[k].im - t.im;
    }
    free(even);
    free(odd);
}

goertzel_cpx_t *fft_radix2(const double *x, size_t n) {
    goertzel_cpx_t *a = malloc(n * sizeof(goertzel_cpx_t));
    for (size_t i = 0; i < n; ++i) {
        a[i].re = x[i];
        a[i].im = 0.0;
    }
    fft_recursive(a, n);
    return a;
}

/* ---------- Referencia: suma directa en long double ---------- */
static long double ref_long_double(const double *x, size_t n, double w) {
    long double sum_re = 0.0L, sum_im = 0.0L;
    for (size_t i = 0; i < n; ++i) {
        long double ang = (long double)w * (long double)i;
        long double cr = cosl(ang);
        long double si = sinl(ang);
        sum_re += (long double)x[i] * cr;
        sum_im -= (long double)x[i] * si;   /* exp(-1i*w*n) = cos - i*sin */
    }
    /* Devolvemos magnitud como double para comparar */
    long double mag = sqrtl(sum_re * sum_re + sum_im * sum_im);
    return (double)mag;
}

/* ---------- Generador de ruido gaussiano (Box-Muller) ---------- */
static double gauss_next(void) {
    static int have = 0;
    static double saved = 0.0;
    if (have) { have = 0; return saved; }
    double u1 = (double)rand() / ((double)RAND_MAX + 1.0);
    double u2 = (double)rand() / ((double)RAND_MAX + 1.0);
    double r = sqrt(-2.0 * log(u1));
    double th = 2.0 * M_PI * u2;
    saved = r * cos(th);
    have = 1;
    return r * sin(th);
}

/* ---------- Benchmark ---------- */
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

int main(void) {
    const size_t N = 1u << 17;   /* 131072 */
    srand(1);

    double *x = malloc(N * sizeof(double));
    for (size_t i = 0; i < N; ++i) x[i] = gauss_next();

    /* Bins a probar */
    size_t bins[] = {1, 3, 10, 100, 1000, N/8, N/4, N/2 - 1};
    size_t n_bins = sizeof(bins) / sizeof(bins[0]);

    printf("N = %zu, ruido gaussiano. Error relativo en magnitud vs suma directa (long double)\n", N);
    printf("%7s %12s %12s %12s %12s %12s\n",
           "k", "omega", "K=2cos", "alpha", "beta", "hibrida");

    for (size_t b = 0; b < n_bins; ++b) {
        size_t k = bins[b];
        double w = 2.0 * M_PI * (double)k / (double)N;
        double R = ref_long_double(x, N, w);

        goertzel_cpx_t cK = goertzel_K(x, N, w);
        goertzel_cpx_t cA = goertzel_alpha(x, N, w);
        goertzel_cpx_t cB = goertzel_beta(x, N, w);
        goertzel_cpx_t cH = goertzel_hybrid(x, N, w);

        double eK = fabs(goertzel_mag(&cK) - R) / R;
        double eA = fabs(goertzel_mag(&cA) - R) / R;
        double eB = fabs(goertzel_mag(&cB) - R) / R;
        double eH = fabs(goertzel_mag(&cH) - R) / R;

        printf("%7zu %12.2e %12.1e %12.1e %12.1e %12.1e\n",
               k, w, eK, eA, eB, eH);
    }

    /* Timing */
    const int rep = 20;
    double w = 2.0 * M_PI * 1000.0 / (double)N;

    /* calentamiento */
    for (int i = 0; i < 3; ++i) {
        goertzel_K(x, N, w);
        goertzel_alpha(x, N, w);
        goertzel_beta(x, N, w);
        goertzel_hybrid(x, N, w);
    }

    double tK = 0, tA = 0, tB = 0, tH = 0;
    for (int i = 0; i < rep; ++i) {
        double t0 = now_ms(); goertzel_K(x, N, w);     tK += now_ms() - t0;
        t0 = now_ms(); goertzel_alpha(x, N, w);        tA += now_ms() - t0;
        t0 = now_ms(); goertzel_beta(x, N, w);         tB += now_ms() - t0;
        t0 = now_ms(); goertzel_hybrid(x, N, w);       tH += now_ms() - t0;
    }
    tK /= rep; tA /= rep; tB /= rep; tH /= rep;

    double t0 = now_ms();
    goertzel_cpx_t *X = fft_radix2(x, N);
    double tFFT = now_ms() - t0;
    free(X);

    printf("\nBenchmark (C, -O2, %d repeticiones, N = %zu):\n", rep, N);
    printf("%12s %12s %10s\n", "forma", "ms/ejec", "relativo");
    printf("%12s %12.3f %10.2fx\n", "K=2cos",   tK, 1.0);
    printf("%12s %12.3f %10.2fx\n", "alpha",    tA, tA / tK);
    printf("%12s %12.3f %10.2fx\n", "beta",     tB, tB / tK);
    printf("%12s %12.3f %10.2fx\n", "hibrida",  tH, tH / tK);
    printf("%12s %12.3f        --\n", "FFT rad2", tFFT);
    printf("1 FFT ~= %.0f bins de Goertzel K\n", tFFT / tK);

    free(x);
    return 0;
}