/* hastings.h - Cabecera del módulo Hastings (1955) + Remez
 *
 * Define los tipos de datos y declara las funciones para las aproximaciones
 * polinómicas de funciones elementales (sin, atan, exp, log1p) usando:
 *   - Los juegos de coeficientes publicados por Hastings (1955). Para estas
 *     cuatro funciones esos valores coinciden numéricamente con el
 *     truncamiento de Taylor; lo distintivo de Hastings 1955 son las
 *     formulaciones racionales (A1-A3, B1-B4) que este módulo NO implementa.
 *   - Refinamiento mediante algoritmo de intercambio de Remez (aportación de
 *     este repositorio, no de 1955) para minimizar el error máximo.
 *
 * Ojo con los grados: el polinomio de Hastings para sin y atan tiene la forma
 * x*P(x^2) con P de grado 5, es decir grado 11 en x. Para exp es grado 5
 * (6 coeficientes) y para log1p grado 9 (10 coeficientes). Para comparar con
 * Remez hay que usar el grado equivalente (11 para sin y atan, 5 para exp,
 * 9 para log1p); un Remez de grado 5 no puede mejorar al de sin y atan.
 *
 * Cada función retornable devuelve hastings_result_t con:
 *   value   - valor aproximado
 *   error   - cota de error declarada para el polinomio usado
 *   degree  - grado del polinomio en x (11 para sin y atan, 5 para exp,
 *              9 para log1p)
 */

#ifndef HASTINGS_H
#define HASTINGS_H

#include <stddef.h>

/* ---------- Resultado de una evaluación -------------------------------- */

typedef struct {
    double value;  /* valor aproximado de la función */
    double error;  /* error máximo estimado del polinomio */
    int    degree; /* grado del polinomio usado          */
} hastings_result_t;

/* ---------- Descripción de un polinomio -------------------------------- */

typedef struct {
    const double *coeffs;  /* coeficientes c[0..n-1] (orden descendente para Horner) */
    int           n;       /* número de coeficientes (grado + 1)                      */
    double        range_min; /* extremo inferior del intervalo de validez */
    double        range_max; /* extremo superior del intervalo de validez */
} hastings_poly_t;
/* ---------- Información de una formulación ----------------------------- */

typedef struct {
    const char *name;   /* nombre de la función      */
    int   degree;       /* grado del polinomio       */
    double tol_abs;     /* tolerancia absoluta       */
    double tol_rel;     /* tolerancia relativa       */
    double range_min;   /* intervalo de validez      */
    double range_max;   /* intervalo de validez      */
} hastings_info_t;

/* ---------- Parámetros para el algoritmo de Remez ---------------------- */

typedef double (*remez_func_t)(double);

typedef struct {
    remez_func_t f;           /* función a aproximar       */
    double       a;           /* extremo inferior          */
    double       b;           /* extremo superior          */
    int          degree;      /* grado del polinomio       */
    int          max_iterations; /* iteraciones máximas    */
    double       tolerance;   /* tolerancia de convergencia */

    /* Salida (rellenada por remez_solve) */
    double       *coeffs;     /* coeficientes resultantes  */
    double       max_error;   /* error máximo estimado     */
    int          converged;   /* 1 si convergió, 0 si no   */
    int          iterations;  /* iteraciones realizadas    */
} remez_params_t;

/* ---------- Funciones públicas (Hastings original) --------------------- */

hastings_result_t hastings_sin(double x);
hastings_result_t hastings_atan(double x);
hastings_result_t hastings_exp(double x);
hastings_result_t hastings_log1p(double x);

double hastings_sin_full(double x);
double hastings_atan_full(double x);
double hastings_exp_full(double x);

double hastings_log(double x);
double hastings_asin(double x);
double hastings_acos(double x);
double hastings_atan2(double y, double x);
double hastings_pow(double base, double exp);
double hastings_sqrt(double x);

/* ---------- Información de polinomios ---------------------------------- */

const hastings_poly_t *hastings_get_poly(const char *name);
const hastings_info_t *hastings_info(const char *name);

/* ---------- Remez / Chebyshev ------------------------------------------ */

int   remez_solve(remez_params_t *p);
double remez_evaluate(const remez_params_t *p, double x);
double remez_max_error(const remez_params_t *p, int num_samples);
void  remez_free(remez_params_t *p);

#endif /* HASTINGS_H */