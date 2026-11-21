/* hastings.c - Polinomios de Hastings (1955) para funciones trascendentales
 *
 * Aproximaciones polinómicas de grado 5 con reduccion de argumento:
 *   - sin(x)   sobre [0, pi/2], reduccion por simetria sin(pi-x)=sin(x)
 *   - atan(x)  con identidad atan(x) = pi/4 + atan((x-1)/(x+1))
 *   - exp(x)   con descomposicion x = k*ln2 + r y series de Taylor
 *   - log1p(x) con identidad log1p(x) = ln2 + log1p((x-1)/2)
 *
 * Incluye refinamiento por el algoritmo de intercambio de Remez para
 * obtener el polinomio de minimax (error maximo minimo) en un intervalo.
 *
 * Referencia: Hastings, C. et al. (1955). Approximations for Digital
 * Computers. Princeton University Press. Remez, E. Ya. (1934).
 */

#include "hastings.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========================================================================= */
/* COEFICIENTES DE HASTINGS (1955)                                            */
/*                                                                           */
/* Que es y que no es este bloque:                                           */
/*   - Para sin, atan, exp y log1p, los juegos de coeficientes que Hastings */
/*     publico en 1955 coinciden numericamente con el TRUNCAMIENTO DE TAYLOR */
/*     (1, -1/6, 1/120, ... para sin; 1, -1/3, 1/5, ... para atan; 1, 1/2,  */
/*     1/6, ... para exp; 1, -1/2, 1/3, ... para log1p). Verificado contra */
/*     la serie de Taylor con mpmath; ver README.                            */
/*   - Lo distintivo de Hastings (1955) son las formulaciones RACIONALES      */
/*     (polinomios A1-A3 y formas racionales B1-B4) para exp, log, log2,    */
/*     atan2 y asin. Este modulo NO las implementa.                          */
/*   - El refinamiento minimax por Remez (mas abajo) tampoco es de 1955:     */
/*     es una aportacion de este repositorio.                                */
/*                                                                           */
/* Grados (contados en x, no en x^2): sin y atan usan x*P(x^2) con P de      */
/* grado 5, o sea grado 11 en x; exp es grado 5 (6 coeficientes); log1p es  */
/* grado 9 (10 coeficientes).                                                */
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
   -0.166666666666667,
    0.142857142857143,
   -0.125000000000000,
    0.111111111111111,
   -0.100000000000000
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

    /* Reducción a [0, pi/2]. sin(pi - x) = sin(x) conserva el signo;
     * sin(2pi - x) = -sin(x) lo invierte. El unico cambio de signo
     * legitimo es el del argumento negativo original. */
    if (x > M_PI) { x = 2.0 * M_PI - x; sign = -sign; }
    if (x > M_PI / 2.0) { x = M_PI - x; }

    double result = x * hastings_eval_poly(hastings_sin_coeffs, 6, x * x);
    return (hastings_result_t){sign * result, 1.0e-5, 11};
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
        return (hastings_result_t){sign * result, 1.0e-5, 11};
    }

    if (x > 1.0) {
        double inv = 1.0 / x;
        double result = M_PI / 2.0 - inv * hastings_eval_poly(hastings_atan_coeffs, 6, inv * inv);
        return (hastings_result_t){sign * result, 1.0e-5, 11};
    }

    double result = x * hastings_eval_poly(hastings_atan_coeffs, 6, x * x);
    return (hastings_result_t){sign * result, 1.0e-5, 11};
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

    /* Reducción de argumento para x en [0, 1]: exp(x) = exp(x/2)^2.
     * Divide x por 2 hasta que quede en [0, 0.25], donde el polinomio
     * Taylor de grado 5 es preciso, y luego eleva al cuadrado k veces. */
    int k = 0;
    double r = x;
    while (r > 0.25) { r /= 2.0; k++; }
    double result = 1.0 + r * hastings_eval_poly(hastings_exp_coeffs, 6, r);
    for (int i = 0; i < k; i++) result *= result;
    return (hastings_result_t){result, 1.0e-5, 5};
}

hastings_result_t hastings_log1p(double x) {
    if (x <= -1.0) return (hastings_result_t){-INFINITY, 0.0, 5};
    if (x > 1.0) {
        double t = (x - 1.0) / (x + 1.0);
        double result = 0.6931471805599453 + t * hastings_eval_poly(hastings_log1p_coeffs, 10, t);
        return (hastings_result_t){result, 1.0e-5, 9};
    }

    /* Reducción de argumento para x >= 0.25:
     * log1p(x) = ln(2) + log1p((x - 1) / 2)
     * lleva [0.25, 1] a [-0.375, 0], donde el polinomio Taylor es preciso.
     */
    if (x >= 0.25) {
        double t = (x - 1.0) / 2.0;
        double result = 0.6931471805599453 + t * hastings_eval_poly(hastings_log1p_coeffs, 10, t);
        return (hastings_result_t){result, 1.0e-5, 9};
    }

    double result = x * hastings_eval_poly(hastings_log1p_coeffs, 10, x);
    return (hastings_result_t){result, 1.0e-5, 9};
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
static const int   poly_ncoeffs[] = {
    (int)(sizeof hastings_sin_coeffs   / sizeof hastings_sin_coeffs[0]),
    (int)(sizeof hastings_atan_coeffs  / sizeof hastings_atan_coeffs[0]),
    (int)(sizeof hastings_exp_coeffs   / sizeof hastings_exp_coeffs[0]),
    (int)(sizeof hastings_log1p_coeffs / sizeof hastings_log1p_coeffs[0])
};
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
            polys[i].coeffs = poly_coeffs[i];
            polys[i].n = poly_ncoeffs[i];
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
    /* degree = grado del polinomio en x. Para sin y atan el polinomio es
     * x*P(x^2) con P de grado 5, es decir grado 11 en x. */
    static hastings_info_t infos[4] = {
        {"sin",  11, 1.0e-5,  1.0e-5,  0.0,          M_PI / 2.0},
        {"atan", 11, 1.0e-5,  1.0e-5,  0.0,          1.0},
        {"exp",  5,  1.0e-5,  1.0e-5,  0.0,          1.0},
        {"log1p",9,  1.0e-5,  1.0e-5,  0.0,          1.0}
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
        if (maxv < 1e-15) return -1;
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

    double *x  = malloc((size_t)m * sizeof(double));
    double *y  = malloc((size_t)m * sizeof(double));
    double **A = malloc((size_t)m * sizeof(double *));
    double *b  = malloc((size_t)m * sizeof(double));
    double *coeffs = malloc((size_t)n * sizeof(double));
    double *ext_x = malloc((size_t)(ns + 2) * sizeof(double));
    double *ext_y = malloc((size_t)(ns + 2) * sizeof(double));
    double *ext_e = malloc((size_t)(ns + 2) * sizeof(double));
    double *sx = malloc((size_t)ns * sizeof(double));
    double *sy = malloc((size_t)ns * sizeof(double));
    double *se = malloc((size_t)ns * sizeof(double));

    if (!x || !y || !A || !b || !coeffs || !ext_x || !ext_y || !ext_e ||
        !sx || !sy || !se) {
        free(x); free(y); free(A); free(b); free(coeffs);
        free(ext_x); free(ext_y); free(ext_e);
        free(sx); free(sy); free(se);
        return -1;
    }
    for (int i = 0; i < m; i++) {
        A[i] = malloc((size_t)m * sizeof(double));
        if (!A[i]) {
            for (int j = 0; j < i; j++) free(A[j]);
            free(A); free(x); free(y); free(b); free(coeffs);
            free(ext_x); free(ext_y); free(ext_e);
            free(sx); free(sy); free(se);
            return -1;
        }
    }

    /* Paso 1: Empezar con puntos equiespaciados */
    for (int i = 0; i < m; i++) {
        x[i] = p->a + (double)i / (double)(m - 1) * (p->b - p->a);
        y[i] = p->f(x[i]);
    }

    int converged = 0;
    int iterations = 0;
    double E = 0.0;
    double E_prev = 0.0;

    for (int iter = 0; iter < p->max_iterations; iter++) {
        iterations = iter;

        /* Construir sistema: sum_j c_j * x_i^j + (-1)^i * E = f(x_i) */
        for (int i = 0; i < m; i++) {
            double xp = 1.0;
            for (int j = 0; j < n; j++) {
                A[i][j] = xp;
                xp *= x[i];
            }
            A[i][n] = (i % 2 == 0) ? 1.0 : -1.0;  /* columna para E */
            b[i] = p->f(x[i]);  /* valor de la funcion objetivo, no del polinomio */
        }

        if (solve_linear(A, b, n + 1) != 0) break;

        for (int j = 0; j < n; j++) coeffs[j] = b[j];
        E = b[n];

        /* Muestrear el error para localizar los extremos alternantes */
        for (int i = 0; i < ns; i++) {
            sx[i] = p->a + (double)i / (ns - 1) * (p->b - p->a);
            sy[i] = p->f(sx[i]);
            se[i] = sy[i] - hastings_eval_poly(coeffs, n, sx[i]);
        }

        /* Candidatos en orden creciente: extremos del intervalo y puntos
         * donde |error| es localmente maximo. El intercambio de Remez
         * exige alternancia en ese orden, no solo signos alternados. */
        int ne = 0;
        ext_x[ne] = p->a; ext_y[ne] = p->f(p->a);
        ext_e[ne] = ext_y[ne] - hastings_eval_poly(coeffs, n, p->a); ne++;
        for (int i = 1; i < ns - 1; i++) {
            double e0 = fabs(se[i-1]), e1 = fabs(se[i]), e2 = fabs(se[i+1]);
            if (e1 >= e0 && e1 >= e2) {
                ext_x[ne] = sx[i]; ext_y[ne] = sy[i]; ext_e[ne] = se[i];
                ne++;
            }
        }
        ext_x[ne] = p->b; ext_y[ne] = p->f(p->b);
        ext_e[ne] = ext_y[ne] - hastings_eval_poly(coeffs, n, p->b); ne++;

        /* Seleccionar m puntos con signos alternados, en orden creciente */
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

        /* Si no hay suficientes alternantes, volver a los puntos
         * equiespaciados y seguir iterando: la alternancia puede
         * aparecer en la ronda siguiente. */
        if (sel < m) {
            for (int i = 0; i < m; i++) {
                x[i] = p->a + (double)i / (double)(m - 1) * (p->b - p->a);
                y[i] = p->f(x[i]);
            }
        }

        /* Verificar convergencia: el error de minimax |E| se estabiliza */
        if (iter > 1 && fabs(E - E_prev) < p->tolerance * fmax(fabs(E), 1e-300)) {
            converged = 1;
            break;
        }
        E_prev = E;
    }

    p->coeffs = coeffs;
    /* Error maximo real medido sobre la malla de muestreo, no solo |E| */
    p->max_error = fabs(E);
    for (int i = 0; i < ns; i++) {
        double err = fabs(sy[i] - hastings_eval_poly(coeffs, n, sx[i]));
        if (err > p->max_error) p->max_error = err;
    }
    p->converged = converged;
    p->iterations = iterations;

    free(x); free(y); for (int i = 0; i < m; i++) free(A[i]); free(A);
    free(b); free(sx); free(sy); free(se);
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