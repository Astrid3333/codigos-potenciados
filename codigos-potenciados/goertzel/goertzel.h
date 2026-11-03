/* hastings.c - Implementaciones de algoritmos matemáticos históricos y modernos
 *
 * Implementa:
 *   - Goertzel K=2cos(w), α=4sin²(w/2), β=4cos²(w/2) (espejo para w cerca de π)
 *   - goertzel_hybrid: α si w < π/2, β si w ≥ π/2
 *   - Formas K, α, β de Goertzel para cálculo de coeficientes DFT
 *
 * Referencia experimental: N = 131072, ruido gaussiano, referencia long double.
 *   bin 1  (ω=4.8e-05):  K=4.8e-08,  α=1.7e-13, híbrida=1.7e-13
 *   bin 10 (w=4.8e-04):  K=2.3e-09,  α=5.4e-15, híbrida=5.4e-15
 *   bin 1000 (w=4.8e-02): K=4.9e-12, α=2.7e-14, híbrida=2.7e-14
 *   π/2 (k=32768):     K=3.5e-15,  α=6.4e-13, híbrida=6.1e-14
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

#ifndef GOERTZEL_H
#define GOERTZEL_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    double re;
    double im;
} goertzel_cpx_t;

goertzel_cpx_t goertzel_K(const double *x, size_t n, double w);
goertzel_cpx_t goertzel_alpha(const double *x, size_t n, double w);
goertzel_cpx_t goertzel_beta(const double *x, size_t n, double w);
goertzel_cpx_t goertzel_hybrid(const double *x, size_t n, double w);
double goertzel_mag(const goertzel_cpx_t *c);

#endif /* GOERTZEL_H */