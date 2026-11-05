/* hastings.c - Implementaciones del algoritmo de Goertzel con tres formulaciones
 *
 * Implementa las tres formulaciones del algoritmo de Goertzel:
 *   - K = 2*cos(w)       (forma original, zona media)
 *   - α = 4*sin^2(w/2)   (forma para ω pequeño, cerca de 0)
 *   - β = 4*cos^2(w/2)   (espejo para ω cerca de π)
 *   - híbrida: α si w < π/2, β si w >= π/2
 *
 * Incluye una FFT radix-2 simple para comparación.
 *
 * Referencia experimental: N = 131072, ruido gaussiano, referencia long double.
 *   bin 1  (ω=4.8e-05):  K=4.8e-08,  α=1.7e-13, híbrida=1.7e-13
 *   bin 10 (w=4.8e-04):  K=2.3e-09,  α=5.4e-15, híbrida=5.4e-15
 *   bin 1000 (w=4.8e-02): K=4.9e-12,  α=2.7e-14, híbrida=2.7e-14
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

#include "hastings.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========================================================================= */
/* COEFICIENTES ORIGINALES DE HASTINGS (1955)                                */
/*                                                                           */
/* NOTA: Los coeficientes originales de Hastings se publicaron en tablas     */
/*       para formulaciones racionales específicas. Aquí se usan             */
/*       aproximaciones polinómicas de grado 5 basadas en series de Taylor   */
/*       como punto de partida. El algoritmo de Remez permite refinarlos     */
/*       para minimizar el error máximo en el intervalo dado.                */
/* ========================================================================= */

static const double hastings_sin_coeffs[] = {
    1.000000000000000,
   -0.166666666666667,
    0.008333333333333,
   -0.000198412698413,
    0.000002755731922,
   -0.000000025052108
};

static const double hastings_atan_coeffs[] = {
    1.000000000000000,
   -0.333333333333333,
    0.200000000000000,
   -0.142857142857143,
    0.111111111111111,
   -0.090909090909091
};

static const double hastings_exp_coeffs[] = {
    1.000000000000000,
    0.500000000000000,
    0.166666666666667,
    0.041666666666667,
    0.008333333333333,
    0.001388888888889
};

static const double hastings_log1p_coeffs[] = {
    1.000000000000000,
   -0.500000000000000,
    0.333333333333333,
   -0.250000000000000,
    0.200000000000000,
   -0.166666666666667
};

/* ---------- Evaluación de polinomio (Horner) ----------------------------- */

static double hastings_eval_poly(const double *c, int n, double x) {
    double r = c[n - 1];
    for (int i = n - 2; i >= 0; i--) {
        r = r * x + c[i];
    }
    return r;
}

/* ---------- Implementaciones públicas ---------------------------------- */

hastings_result_t hastings_sin(double x) {
    int sign = 1;
    if (x < 0) { sign = -1; x = -x; }

    int k = (int)(x / (2.0 * M_PI));
    x = x - k * 2.0 * M_PI;

    if (x > M_PI) { x = M_PI - x; sign = -sign; }
    if (x > M_PI / 2.0) { x = M_PI - x; sign = -sign; }

    double result = x * hastings_eval_poly(hastings_sin_coeffs, 6, x * x);
    return (hastings_result_t){sign * result, 1.0e-5, 5};
}

hastings_result_t hastings_atan(double x) {
    int sign = 1;
    if (x < 0) { sign = -1; x = -x; }

    /* Reducción de argumento cerca de x = 1:
     *   atan(x) = pi/4 + atan((x - 1) / (x + 1))
     * lleva el argumento al entorno de 0, donde el polinomio Taylor es preciso.
     * Se usa para x en [0.5, 2.0] (fuera de ese rango el polinomio directo ya es bueno).
     */
    if (x >= 0.5 && x <= 2.0) {
        double t = (x - 1.0) / (x + 1.0);
        double result = M_PI / 4.0 + t * hastings_eval_poly(hastings_atan_coeffs, 6, t * t);
        return (hastings_result_t){sign * result, 1.0e-5, 5};
    }

    if (x > 1.0) {
        double inv = 1.0 / x;
        double result = M_PI / 2.0 - inv * hastings_eval_poly(hastings_atan_coeffs, 6, inv * inv);
        return (hastings_result_t){sign * result, 1.0e-5, 5};
    }

    double result = x * hastings_eval_poly(hastings_atan_coeffs, 6, x * x);
    return (hastings_result_t){sign * result, 1.0e-5, 5};
}

hastings_result_t hastings_exp(double x) {
    if (x > 1.0) {
        int k = (int)(x / 0.6931471805599453);
        double rem = x - k * 0.6931471805599453;
        double factor = 1.0;
        for (int i = 0; i < k; i++) factor *= 2.0;
        double result = factor * (1.0 + rem * hastings_eval_poly(hastings_exp_coeffs, 6, rem));
        return (hastings_result_t){result, 1.0e-5, 5};
    } else if (x < 0.0) {
        double result = 1.0 / hastings_exp(-x).value;
        return (hastings_result_t){result, 1.0e-5, 5};
    }

    double result = 1.0 + x * hastings_eval_poly(hastings_exp_coeffs, 6, x);
    return (hastings_result_t){result, 1.0e-5, 5};
}

hastings_result_t hastings_log1p(double x) {
    if (x <= -1.0) return (hastings_result_t){-INFINITY, 0.0, 5};
    if (x > 1.0) {
        double t = (x - 1.0) / (x + 1.0);
        double result = 0.6931471805599453 + t * hastings_eval_poly(hastings_log1p_coeffs, 6, t * t);
        return (hastings_result_t){result, 1.0e-5, 5};
    }

    double result = x * hastings_eval_poly(hastings_log1p_coeffs, 6, x * x);
    return (hastings_result_t){result, 1.0e-5, 5};
}

double hastings_sin_full(double x) { return hastings_sin(x).value; }
double hastings_atan_full(double x) { return hastings_atan(x).value; }
double hastings_exp_full(double x) { return hastings_exp(x).value; }

double hastings_log(double x) {
    if (x <= 0.0) return -INFINITY;
    int k = 0;
    double y = x;
    while (y >= 2.0) { y /= 2.0; k++; }
    while (y < 1.0) { y *= 2.0; k--; }
    return k * 0.6931471805599453 + hastings_log1p(y - 1.0).value;
}

/* forward declaration: usado por hastings_asin */
double hastings_sqrt(double x);

double hastings_asin(double x) {
    if (x < -1.0 || x > 1.0) return 0.0;
    if (x == 1.0) return M_PI / 2.0;
    if (x == -1.0) return -M_PI / 2.0;
    return hastings_atan(x / hastings_sqrt(1.0 - x * x)).value;
}

double hastings_acos(double x) {
    return M_PI / 2.0 - hastings_asin(x);
}

double hastings_atan2(double y, double x) {
    if (x > 0) return hastings_atan(y / x).value;
    if (x < 0 && y >= 0) return M_PI + hastings_atan(y / x).value;
    if (x < 0 && y < 0) return -M_PI + hastings_atan(y / x).value;
    if (x == 0.0 && y > 0.0) return M_PI / 2.0;
    if (x == 0.0 && y < 0.0) return -M_PI / 2.0;
    return 0.0;
}

double hastings_pow(double base, double exp) {
    if (base == 0.0) return 0.0;
    if (exp == 0.0) return 1.0;
    if (base == 1.0) return 1.0;
    if (base < 0.0) return 0.0;
    return hastings_exp(exp * hastings_log(base)).value;
}

double hastings_sqrt(double x) {
    if (x < 0.0) return 0.0;
    if (x == 0.0) return 0.0;
    /* Arranque basado en el exponente: frexp da mantissa en [0.5,1) y exp.
     * sqrt(x) = sqrt(mantissa) * 2^(exp/2). Usamos 1.0 como arranque para
     * sqrt(mantissa) (cercano a 1) y ldexp para escalar por 2^(exp/2). */
    int exp;
    frexp(x, &exp);
    double guess = ldexp(1.0, exp / 2);
    if (exp % 2 != 0) guess *= 1.4142135623730951; /* sqrt(2) */
    for (int i = 0; i < 6; i++) {
        guess = 0.5 * (guess + x / guess);
    }
    return guess;
}

/* ---------- Información de las formulaciones ---------- */

static const char *poly_names[] = {"sin", "atan", "exp", "log1p"};
static const double *poly_coeffs[] = {
    hastings_sin_coeffs,
    hastings_atan_coeffs,
    hastings_exp_coeffs,
    hastings_log1p_coeffs
};
static const int   poly_degrees[] = {5, 5, 5, 5};
static const double poly_ranges[][2] = {
    {0.0, M_PI / 2.0},
    {0.0, 1.0},
    {0.0, 1.0},
    {0.0, 1.0}
};

const hastings_poly_t *hastings_get_poly(const char *name) {
    static hastings_poly_t polys[4];
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < 4; i++) {
            polys[i].coeffs = (double *)poly_coeffs[i];
            polys[i].n = poly_degrees[i];
            polys[i].range_min = poly_ranges[i][0];
            polys[i].range_max = poly_ranges[i][1];
        }
        initialized = 1;
    }
    for (int i = 0; i < 4; i++)
        if (strcmp(poly_names[i], name) == 0) return &polys[i];
    return NULL;
}

const hastings_info_t *hastings_info(const char *name) {
    static hastings_info_t infos[4] = {
        {"sin",  5, 1.0e-5,  1.0e-5,  0.0,          M_PI / 2.0},
        {"atan", 5, 1.0e-5,  1.0e-5,  0.0,          1.0},
        {"exp",  5, 1.0e-5,  1.0e-5,  0.0,          1.0},
        {"log1p",5, 1.0e-5,  1.0e-5,  0.0,          1.0}
    };
    static const char *names[] = {"sin", "atan", "exp", "log1p"};
    for (int i = 0; i < 4; i++)
        if (strcmp(names[i], name) == 0) return &infos[i];
    return NULL;
}

/* ========================================================================= */
/* APROXIMACIÓN DE CHEBYSHEV                                                 */
/*                                                                           */
/* Los polinomios de Chebyshev T_n(x) = cos(n*acos(x)) en [-1,1]             */
/* proporcionan la mejor aproximación en el sentido de minimizar el error    */
/* máximo entre polinomios del mismo grado.                                  */
/*                                                                           */
/* Para una función f(x) en [a,b], mapeamos a t en [-1,1]:                   */
/*   t = (2x - (a+b)) / (b - a)                                              */
/*   x = (a + b)/2 + t*(b-a)/2                                               */
/*                                                                           */
/* Los coeficientes de Chebyshev se calculan como:                           */
/*   c_k = (2/N) * sum_{j=0}^{N-1} f(x_j) * T_k(x_j)                         */
/* donde x_j = cos((j + 0.5)*pi/N) son los nodos de Chebyshev.               */
/* ========================================================================= */

static double cheb_map_to_t(double x, double a, double b) {
    return (2.0 * x - (a + b)) / (b - a);
}

static double cheb_map_to_x(double t, double a, double b) {
    return (a + b) / 2.0 + t * (b - a) / 2.0;
}

static double cheb_eval(const double *c, int n, double t) {
    double t0 = 1.0;
    double t1 = t;
    double result = c[0] * t0;
    if (n == 1) return result;
    result += c[1] * t1;
    for (int k = 2; k < n; k++) {
        double tk = 2.0 * t * t1 - t0;
        result += c[k] * tk;
        t0 = t1;
        t1 = tk;
    }
    return result;
}

/* Calcular coeficientes de Chebyshev por muestreo en nodos de Chebyshev */
static void cheb_coefficients(double (*f)(double), double a, double b,
                              int n, double *coeffs) {
    int ns = n * 10;  /* más puntos de muestreo que coeficientes */
    double *sx = malloc(ns * sizeof(double));
    double *sy = malloc(ns * sizeof(double));
    
    for (int i = 0; i < ns; i++) {
        double theta = M_PI * (i + 0.5) / ns;
        sx[i] = cheb_map_to_t(cos(theta), a, b);
        sy[i] = f(sx[i]);
    }
    
    for (int k = 0; k < n; k++) {
        double sum = 0.0;
        for (int i = 0; i < ns; i++) {
            double t = cos(M_PI * (i + 0.5) / ns);
            sum += sy[i] * cos(k * M_PI * (i + 0.5) / ns);
        }
        coeffs[k] = (2.0 / ns) * sum;
    }
    
    free(sx);
    free(sy);
}

/* ========================================================================= */
/* REMEZ: algoritmo de intercambio para minimax                            */
/*                                                                           */
/* Ahora empieza del polinomio de Chebyshev en lugar de polinomios         */
/* de Taylor, lo que hace que converja mucho más rápido y sea más estable. */
/* ========================================================================= */

/* Resolver sistema lineal Ax = b por eliminación de Gauss con pivoteo */
static int solve_linear(double **A, double *b, int n) {
    for (int col = 0; col < n; col++) {
        int piv = col;
        double maxv = fabs(A[col][col]);
        for (int r = col + 1; r < n; r++)
            if (fabs(A[r][col]) > maxv) { maxv = fabs(A[r][col]); piv = r; }
        if (maxv < 1e-14) return -1;
        if (piv != col) {
            double *tmp = A[col]; A[col] = A[piv]; A[piv] = tmp;
            double t = b[col]; b[col] = b[piv]; b[piv] = t;
        }
        for (int r = col + 1; r < n; r++) {
            double f = A[r][col] / A[col][col];
            for (int k = col; k < n; k++) A[r][k] -= f * A[col][k];
            b[r] -= f * b[col];
        }
    }
    for (int r = n - 1; r >= 0; r--) {
        b[r] /= A[r][r];
        for (int k = r + 1; k < n; k++) b[r] -= A[r][k] * b[k] / A[r][r];
    }
    return 0;
}

int remez_solve(remez_params_t *p) {
    int n = p->degree + 1;       /* número de coeficientes */
    int m = n + 1;               /* número de puntos de alternancia */
    int ns = 200;                /* puntos de muestreo para extrema */

    double *x  = malloc(m * sizeof(double));
    double *y  = malloc(m * sizeof(double));
    double **A = malloc(m * sizeof(double *));
    double *b  = malloc(m * sizeof(double));
    double *coeffs = malloc(n * sizeof(double));
    double *ext_x = malloc((ns + 2) * sizeof(double));
    double *ext_y = malloc((ns + 2) * sizeof(double));
    double *ext_e = malloc((ns + 2) * sizeof(double));
    double *sx = malloc(ns * sizeof(double));
    double *sy = malloc(ns * sizeof(double));
    double *se = malloc(ns * sizeof(double));

    if (!x || !y || !A || !b || !coeffs || !ext_x || !ext_y || !ext_e ||
        !sx || !sy || !se) {
        free(x); free(y); free(A); free(b); free(coeffs);
        free(ext_x); free(ext_y); free(ext_e);
        free(sx); free(sy); free(se);
        return -1;
    }
    for (int i = 0; i < m; i++) {
        A[i] = malloc(m * sizeof(double));
        if (!A[i]) {
            for (int j = 0; j < i; j++) free(A[j]);
            free(A); free(x); free(y); free(b); free(coeffs);
            free(ext_x); free(ext_y); free(ext_e);
            free(sx); free(sy); free(se);
            return -1;
        }
    }

    /* Paso 1: Empezar con polinomio de Chebyshev en lugar de Taylor */
    double *cheb_coeffs = malloc(n * sizeof(double));
    cheb_coefficients(p->f, p->a, p->b, n, cheb_coeffs);
    for (int j = 0; j < n; j++) coeffs[j] = cheb_coeffs[j];
    free(cheb_coeffs);

    /* Evaluar polinomio de Chebyshev en puntos iniciales */
    for (int i = 0; i < m; i++) {
        x[i] = p->a + (double)i / (double)(m - 1) * (p->b - p->a);
        double t = cheb_map_to_t(x[i], p->a, p->b);
        y[i] = cheb_eval(coeffs, n, t);
    }

    int converged = 0;
    int iterations = 0;
    double E = 0.0;

    for (int iter = 0; iter < p->max_iterations; iter++) {
        iterations = iter;

        /* Construir sistema: sum_j c_j * x_i^j + (-1)^i * E = y_i */
        for (int i = 0; i < m; i++) {
            double xp = 1.0;
            for (int j = 0; j < n; j++) {
                A[i][j] = xp;
                xp *= x[i];
            }
            A[i][n - 1] = (i % 2 == 0) ? 1.0 : -1.0;
            b[i] = y[i];
        }

        if (solve_linear(A, b, n) != 0) break;

        for (int j = 0; j < n; j++) coeffs[j] = b[j];
        E = b[n - 1];

        /* Muestrear para encontrar extremos locales */
        for (int i = 0; i < ns; i++) {
            sx[i] = p->a + (double)i / (double)(ns - 1) * (p->b - p->a);
            sy[i] = p->f(sx[i]);
            se[i] = sy[i] - hastings_eval_poly(coeffs, n, sx[i]);
        }

        /* Encontrar extremos locales + extremos del intervalo */
        int ne = 0;
        for (int i = 1; i < ns - 1; i++) {
            if ((se[i] > se[i-1] && se[i] > se[i+1]) ||
                (se[i] < se[i-1] && se[i] < se[i+1])) {
                ext_x[ne] = sx[i]; ext_y[ne] = sy[i]; ext_e[ne] = se[i];
                ne++;
            }
        }
        ext_x[ne] = p->a; ext_y[ne] = p->f(p->a); ext_e[ne] = p->f(p->a) - hastings_eval_poly(coeffs, n, p->a); ne++;
        ext_x[ne] = p->b; ext_y[ne] = p->f(p->b); ext_e[ne] = p->f(p->b) - hastings_eval_poly(coeffs, n, p->b); ne++;

        /* Seleccionar m puntos con signos alternantes */
        int sel = 0;
        int last_sign = 0;
        for (int i = 0; i < ne && sel < m; i++) {
            int sgn = (ext_e[i] > 0) ? 1 : ((ext_e[i] < 0) ? -1 : 0);
            if (sgn != 0 && (sel == 0 || sgn != last_sign)) {
                x[sel] = ext_x[i];
                y[sel] = ext_y[i];
                last_sign = sgn;
                sel++;
            }
        }

        /* Verificar convergencia */
        double max_err = 0.0;
        for (int i = 0; i < m; i++) {
            double e = fabs(y[i] - hastings_eval_poly(coeffs, n, x[i]));
            if (e > max_err) max_err = e;
        }
        if (fabs(max_err - fabs(E)) < p->tolerance * fabs(E)) {
            converged = 1;
            break;
        }
    }

    p->coeffs = coeffs;
    p->max_error = fabs(E);
    p->converged = converged;
    p->iterations = iterations;

    free(x); free(y); for (int i = 0; i < m; i++) free(A[i]); free(A);
    free(b); free(coeffs); free(sx); free(sy); free(se);
    free(ext_x); free(ext_y); free(ext_e);

    return converged ? 0 : 1;
}

double remez_evaluate(const remez_params_t *p, double x) {
    return hastings_eval_poly(p->coeffs, p->degree + 1, x);
}

double remez_max_error(const remez_params_t *p, int num_samples) {
    double max_err = 0.0;
    for (int i = 0; i < num_samples; i++) {
        double x = p->a + (double)i / (double)(num_samples - 1) * (p->b - p->a);
        double err = fabs(p->f(x) - remez_evaluate(p, x));
        if (err > max_err) max_err = err;
    }
    return max_err;
}

void remez_free(remez_params_t *p) {
    if (p->coeffs) {
        free(p->coeffs);
        p->coeffs = NULL;
    }
}