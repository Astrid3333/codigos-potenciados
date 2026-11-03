/* hastings.h - Polinomios de Hastings (1955) con refinamiento por Remez
 *
 * Implementa aproximaciones polinómicas para:
 *   - sin(x)     en [0, pi/2]
 *   - atan(x)    en [0, 1]
 *   - exp(x)     en [0, 1]
 *   - log(1+x)   en [0, 1]
 *
 * Incluye las formulaciones originales de Hastings y un refinamiento
 * mediante el algoritmo de intercambio de Remez para reducir el error
 * máximo o reducir el número de términos manteniendo la precisión.
 */

#ifndef HASTINGS_H
#define HASTINGS_H

#include <stddef.h>

/* Resultado de una evaluación */
typedef struct {
    double value;   /* valor aproximado */
    double error;   /* error absoluto estimado */
    int terms;      /* numero de términos usados */
} hastings_result_t;

/* Polinomio: c[0] + c[1]*x + c[2]*x^2 + ... + c[n-1]*x^(n-1) */
typedef struct {
    const double *coeffs;
    int n;
    double range_min;
    double range_max;
} hastings_poly_t;

/* Informacion de cada formulacion */
typedef struct {
    const char *name;
    int degree;
    double max_error_hastings;  /* error reportado por Hastings (1955) */
    double max_error_remez;     /* error despues de Remez (si se refinó) */
    double range_min;
    double range_max;
} hastings_info_t;

/* ---------- Formulaciones originales de Hastings (1955) ---------- */

hastings_result_t hastings_sin(double x);
hastings_result_t hastings_atan(double x);
hastings_result_t hastings_exp(double x);
hastings_result_t hastings_log1p(double x);

/* Versiones con rango extendido */
double hastings_sin_full(double x);
double hastings_atan_full(double x);
double hastings_exp_full(double x);
double hastings_log(double x);

/* ---------- Informacion de las formulaciones ---------- */

const hastings_poly_t *hastings_get_poly(const char *name);
const hastings_info_t *hastings_info(const char *name);

/* ---------- Remez: refinamiento de minimax ---------- */

typedef struct {
    double (*f)(double x);           /* funcion a aproximar */
    double a;                        /* limite inferior */
    double b;                        /* limite superior */
    int degree;                      /* grado del polinomio */
    int max_iterations;              /* maximo de iteraciones */
    double tolerance;                /* tolerancia de convergencia */

    /* Resultado */
    double *coeffs;                  /* coeficientes c0 + c1*x + ... + cn*x^n */
    double max_error;                /* error maximo absoluto */
    int converged;                   /* 1 si convergio */
    int iterations;                  /* iteraciones realizadas */
} remez_params_t;

int remez_solve(remez_params_t *params);
void remez_free(remez_params_t *params);
double remez_evaluate(const remez_params_t *p, double x);
double remez_max_error(const remez_params_t *p, int num_samples);

#endif /* HASTINGS_H */