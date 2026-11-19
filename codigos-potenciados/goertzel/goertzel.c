/* goertzel.c - Implementaciones del algoritmo de Goertzel con tres formulaciones
 *
 * Implementa las tres formulaciones del algoritmo de Goertzel:
 *   - K = 2*cos(w)       (forma original, zona media)
 *   - α = 4*sin^2(w/2)   (forma para ω pequeño, cerca de 0)
 *   - β = 4*cos^2(w/2)   (espejo para w cerca de π)
 *   - híbrida: 3 zonas — α si w < π/4, K si π/4 ≤ w ≤ 3π/4, β si w > 3π/4
 *
 * Incluye una FFT radix-2 simple para comparación.
 *
 * Referencia experimental: N = 131072, ruido gaussiano, referencia long double.
 *   bin 1  (ω=4.8e-05):  K=3.9e-06,  α=4.7e-13, β=2.2e-05
 *   bin 10 (w=4.8e-04):  K=3.9e-08,  α=1.9e-14, β=6.8e-08
 *   bin 1000 (w=4.8e-02): K=5.1e-12,  α=4.0e-14, β=5.0e-11
 *   π/2 (bin 32768):     K=1.2e-13,  α=1.6e-11, β=1.6e-11
 *   π   (k=65535):       K=1.7e-08,  α=9.4e-08, β=3.8e-13
 *
 * Recomendación práctica (umbrales medidos, no supuestos):
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

/* ---------- Goertzel híbrida: 3 zonas (α / K / β) --------------------- */

/* Umbrales de la formulación por tramos. Medidos con N = 131072 y referencia
 * en long double (ver goertzel_umbrales.txt):
 *   - ω < π/4  : α es 2-4 órdenes de magnitud mejor que K (en bin 1, α da
 *                 4.7e-13 frente a 3.9e-06 de K, porque K = 2cos(ω) pierde
 *                 todos los dígitos significantemente cuando ω → 0).
 *   - π/4 ≤ ω ≤ 3π/4 : K es la más estable; α y β degradan en el centro
 *                 (en ω = π/2 exacto, α y β dan 1.6e-11 mientras K da
 *                 1.2e-13, dos órdenes de magnitud mejor).
 *   - ω > 3π/4 : β es la más estable por simetría con el primer tramo
 *                 (en bin 65535, β da 3.8e-13 frente a 1.7e-08 de K).
 * Los umbrales π/4 y 3π/4 son los puntos donde la forma K empieza a ser
 * competitive con α (respectivamente con β), y coinciden con la recomendación
 * piecewise del informe experimental. */
#define GOERTZEL_ALPHA_MAX_W (M_PI / 4.0)
#define GOERTZEL_BETA_MIN_W  (3.0 * M_PI / 4.0)

goertzel_cpx_t goertzel_hybrid(const double *x, size_t n, double w) {
    /* Plegado por aliasing: para ω > π se usa ω' = 2π - ω y se conjuga el
     * resultado. X[2πk/N] es la conjugada de X[2π(N-k)/N] para señal real, de
     * modo que el cálculo con ω' da el cociente correcto tras conjugar.
     * Sin este plegado la zona ω > π usaría β, que es la forma peor ahí:
     * el peor error en [0, 2π) baja de 2.2e-09 a 1.4e-10 al plegar. */
    int conjugate = 0;
    goertzel_cpx_t c;

    if (w > M_PI) {
        w = 2.0 * M_PI - w;
        conjugate = 1;
    }

    if (w < GOERTZEL_ALPHA_MAX_W)
        c = goertzel_alpha(x, n, w);
    else if (w > GOERTZEL_BETA_MIN_W)
        c = goertzel_beta(x, n, w);
    else
        c = goertzel_K(x, n, w);

    if (conjugate) c.im = -c.im;
    return c;
}

/* ---------- Magnitud de coeficiente complexo -------------------------- */

double goertzel_mag(const goertzel_cpx_t *c) {
    return sqrt(c->re * c->re + c->im * c->im);
}

/* ---------- FFT radix-2 in-place (para comparar con Goertzel) --------- */

int goertzel_es_potencia_de_dos(size_t n) {
    return (n > 0U && (n & (n - 1U)) == 0U) ? 1 : 0;
}

int goertzel_fft(double *re, double *im, size_t n) {
    size_t i, j, m, mmax, istep;
    double theta, wtemp, cr, ci, wr, wi;
    if (!re || !im || !goertzel_es_potencia_de_dos(n)) {
        return -1;
    }

    /* reordenamiento de bits (bit reversal) */
    for (i = 1U, j = 0U; i < n; ++i) {
        size_t bit = n >> 1;
        for (; (j & bit) != 0U; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            double tr = re[i];
            double ti = im[i];
            re[i] = re[j];
            im[i] = im[j];
            re[j] = tr;
            im[j] = ti;
        }
    }

    /* mariposas */
    for (mmax = 1U; mmax < n; mmax = istep) {
        istep = mmax << 1;
        theta = -2.0 * M_PI / (double)istep;
        cr = cos(theta);
        ci = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m = 0U; m < mmax; ++m) {
            for (i = m; i < n; i += istep) {
                j = i + mmax;
                wtemp = wr * re[j] - wi * im[j];
                {
                    double wi2 = wr * im[j] + wi * re[j];
                    re[j] = re[i] - wtemp;
                    im[j] = im[i] - wi2;
                    re[i] += wtemp;
                    im[i] += wi2;
                }
            }
            wtemp = wr;
            wr = wtemp * cr - wi * ci;
            wi = wtemp * ci + wi * cr;
        }
    }
    return 0;
}

double goertzel_fft_mag(const double *re, const double *im, size_t n, size_t k) {
    if (!re || !im || k >= n) {
        return 0.0;
    }
    return sqrt(re[k] * re[k] + im[k] * im[k]);
}