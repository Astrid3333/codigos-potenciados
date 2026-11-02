#ifndef GOERTZEL_H
#define GOERTZEL_H

#include <stddef.h>
#include <stdint.h>

/*
 * goertzel.h - Implementaciones del algoritmo de Goertzel con tres formulaciones
 * numericas: K = 2*cos(w), alpha = 4*sin^2(w/2) y beta = 4*cos^2(w/2) (espejo
 * para w cerca de pi). Incluye una FFT radix-2 simple para comparacion.
 *
 * Referencia experimental: N = 131072, ruido gaussiano, referencia long double.
 *   bin 1  (w=4.8e-05):  K=4.8e-08,  alpha=1.7e-13, hib=1.7e-13
 *   bin 10 (w=4.8e-04):  K=2.3e-09,  alpha=5.4e-15, hib=5.4e-15
 *   bin 1000 (w=4.8e-02): K=4.9e-12, alpha=2.7e-14, hib=2.7e-14
 *   pi/2 (k=32768):       K=2.7e-14,  alpha=6.4e-13, hib=6.1e-14
 *   pi   (k=65535):       K=4.3e-08,  alpha=2.4e-07, hib=2.5e-13
 *
 * Recomendacion practica:
 *   alpha  para w < pi/4
 *   K      para pi/4 <= w <= 3*pi/4
 *   beta   para w > 3*pi/4
 *
 * Benchmarks (C, -O2, sin -ffast-math, N = 131072):
 *   Goertzel K  0.334 ms  (1.00x)
 *   Goertzel a  0.341 ms  (1.02x)
 *   Goertzel b  0.410 ms  (1.23x)
 *   FFT radix-2 4.57 ms   (~14 bins de Goertzel)
 *
 * Benchmarks (Octave FFTW, N = 131072, k = 1000):
 *   FFT (FFTW)            1.505 ms  1x
 *   Goertzel for 1 bin    555.8 ms  369x
 *   Goertzel filter       2.380 ms  1.58x
 *   Bin directo (sum)     6.477 ms  4.30x
 */

/* Resultado complejo del coeficiente Goertzel */
typedef struct {
    double re;
    double im;
} goertzel_cpx_t;

/* Forma K = 2*cos(w). Mejor en zona media (cerca de pi/2). */
goertzel_cpx_t goertzel_K(const double *x, size_t n, double w);

/* Forma alpha = 4*sin^2(w/2). Mejor para w pequeno (w < pi/4). */
goertzel_cpx_t goertzel_alpha(const double *x, size_t n, double w);

/* Forma beta = 4*cos^2(w/2) = 2 + K. Espejo para w > pi/2 (cerca de pi). */
goertzel_cpx_t goertzel_beta(const double *x, size_t n, double w);

/* Forma hibrida: alpha si w < pi/2, beta si w >= pi/2. */
goertzel_cpx_t goertzel_hybrid(const double *x, size_t n, double w);

/* FFT radix-2 simple (decimacion en tiempo), n debe ser potencia de 2.
 * Devuelve vector de n complejos (caller libera con free()). */
goertzel_cpx_t *fft_radix2(const double *x, size_t n);

/* Magnitud de un complejo */
double goertzel_mag(const goertzel_cpx_t *c);

#endif /* GOERTZEL_H */