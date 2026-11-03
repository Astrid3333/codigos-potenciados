/* hastings.c - Formulaciones de Goertzel K, α, β, híbrida
 *
 * Implementa las tres formulaciones del algoritmo de Goertzel:
 *   - K = 2*cos(w)       (forma original, zona media)
 *   - α = 4*sin^2(w/2)   (forma para ω pequeño, cerca de 0)
 *   - β = 4*cos^2(w/2)   (espejo para ω cerca de π)
 *   - híbrida: α si w < π/2, β si w ≥ π/2
 *
 * Incluye una FFT radix-2 simple para comparación.
 *
 * Referencia experimental: N = 131072, ruido gaussiano, referencia long double.
 *   bin 1  (ω=4.8e-05):  K=4.8e-08,  α=1.7e-13, híbrida=1.7e-13
 *   bin 10 (w=4.8e-04):  K=2.3e-09,  α=5.4e-15, híbrida=5.4e-15
 *   bin 1000 (w=4.8e-02): K=4.9e-12, α=2.7e-14, híbrida=2.7e-14
 *   π/2 (bin 32768):     K=3.5e-15,  α=6.4e-13, híbrida=6.1e-14
 *   π   (k=65535):       K=4.3e-08,  α=2.4e-07, híbrida=2.5e-13
 *
 * Recomendación práctica:
 *   α  para ω < π/4
 *   K  para π/4 ≤ ω ≤ 3π/4
 *   β  para ω > 3π/4
 *
 * Benchmarks (C, -O2, sin -ffast-math, N = 131072):
 *   Goertzel K  0.334 ms  (1.00x)
 *   Goertzel α  0.341 ms  (1.02x)
 *   Goertzel β  0.410 ms  (1.23x)
 *   FFT radix-2 4.57 ms   (~14 bins de Goertzel)
 *
 * Benchmarks (Octave FFTW, N = 131072, k = 1000):
 *   FFT (FFTW)            1.505 ms  1x
 *   Goertzel for 1 bin    555.8 ms  369x
 *   Goertzel filter       2.380 ms  1.58x
 *   Bin directo (sum)     6.477 ms  4.30x
 */

#include "goertzel.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- Goertzel K: forma original ----------------------------- */

goertzel_cpx_t goertzel_K(const double *x, size_t n, double w) {
    double K = 2.0 * cos(w);
    double s1 = 0.0, s2 = 0.0, s;
    for (size_t i = 0; i < n; ++i) {
        s = x[i] + K * s1 - s2;
        s2 = s1;
        s1 = s;
    }
    s = K * s1 - s2;  /* x[N] = 0 */
    return (goertzel_cpx_t){ s - cos(w) * s1, sin(w) * s1 };
}

/* ---------- Goertzel α: forma para ω pequeño -------------------------- */

goertzel_cpx_t goertzel_alpha(const double *x, size_t n, double w) {
    double al = 4.0 * sin(w / 2.0) * sin(w / 2.0);
    double s = 0.0, d = 0.0;
    for (size_t i = 0; i < n; ++i) {
        d = d + x[i] - al * s;
        s = s + d;
    }
    d = d - al * s;
    s = s + d;
    double s1 = s - d;
    return (goertzel_cpx_t){ s - cos(w) * s1, sin(w) * s1 };
}

/* ---------- Goertzel β: espejo para w cerca de π ---------------------- */

goertzel_cpx_t goertzel_beta(const double *x, size_t n, double w) {
    double be = 4.0 * cos(w / 2.0) * cos(w / 2.0);
    double s = 0.0, d = 0.0;
    for (size_t i = 0; i < n; ++i) {
        d = x[i] + be * s - d;
        s = d - s;
    }
    d = be * s - d;
    s = d - s;
    double s1 = d - s;
    return (goertzel_cpx_t){ s - cos(w) * s1, sin(w) * s1 };
}

/* ---------- Goertzel híbrida: α si w < π/2, β si w >= π/2 ------------- */

goertzel_cpx_t goertzel_hybrid(const double *x, size_t n, double w) {
    if (w < M_PI / 2.0)
        return goertzel_alpha(x, n, w);
    return goertzel_beta(x, n, w);
}

/* ---------- Magnitud de coeficiente complexo -------------------------- */

double goertzel_mag(const goertzel_cpx_t *c) {
    return sqrt(c->re * c->re + c->im * c->im);
}