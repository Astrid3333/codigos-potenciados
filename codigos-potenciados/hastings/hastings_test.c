/* hastings_test.c - Pruebas de precisión para Hastings (1955) y Remez
 *
 * Compara las implementaciones originales de Hastings con las refinadas
 * por el algoritmo de intercambio de Remez (empezando de Chebyshev),
 * midiendo el error máximo contra las funciones de long double.
 *
 * Compilar: gcc -std=c11 -O2 hastings.c hastings_test.c -o hastings_test -lm
 * Ejecutar: ./hastings_test
 */

#include "hastings.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- Funciones auxiliares de error máximo ---------- */

static double max_err_sin(double a, double b, int n) {
    double max_err = 0.0;
    double max_x = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double ref = sin(x);
        double got = hastings_sin(x).value;
        double err = fabs(ref - got);
        if (err > max_err) { max_err = err; max_x = x; }
    }
    printf("    x con error max: %.10f  error: %.3e\n", max_x, max_err);
    return max_err;
}

static double max_err_atan(double a, double b, int n) {
    double max_err = 0.0;
    double max_x = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double ref = atan(x);
        double got = hastings_atan(x).value;
        double err = fabs(ref - got);
        if (err > max_err) { max_err = err; max_x = x; }
    }
    printf("    x con error max: %.10f  error: %.3e\n", max_x, max_err);
    return max_err;
}

static double max_err_exp(double a, double b, int n) {
    double max_err = 0.0;
    double max_x = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double ref = exp(x);
        double got = hastings_exp(x).value;
        double err = fabs(ref - got);
        if (err > max_err) { max_err = err; max_x = x; }
    }
    printf("    x con error max: %.10f  error: %.3e\n", max_x, max_err);
    return max_err;
}

static double max_err_log1p(double a, double b, int n) {
    double max_err = 0.0;
    double max_x = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double ref = log1p(x);
        double got = hastings_log1p(x).value;
        double err = fabs(ref - got);
        if (err > max_err) { max_err = err; max_x = x; }
    }
    printf("    x con error max: %.10f  error: %.3e\n", max_x, max_err);
    return max_err;
}

/* ---------- Pruebas ---------- */

static void test_sin(void) {
    printf("=== sin(x) en [0, pi/2] ===\n");
    printf("  Hastings original:  ");
    double e1 = max_err_sin(0.0, M_PI/2.0, 2000);

    remez_params_t rp = {0};
    rp.f = sin;
    rp.a = 0.0;
    rp.b = M_PI/2.0;
    rp.degree = 5;
    rp.max_iterations = 200;
    rp.tolerance = 1e-10;

    if (remez_solve(&rp) == 0) {
        printf("  Remez refinado:   ");
        double e2 = remez_max_error(&rp, 2000);
        printf("    error max: %.3e\n", e2);
        printf("    mejora:    %.1fx\n", e1 / e2);
        remez_free(&rp);
    } else {
        printf("  Remez: no convergió (%d iters)\n", rp.iterations);
    }
    printf("\n");
}

static void test_atan(void) {
    printf("=== atan(x) en [0, 1] ===\n");
    printf("  Hastings original:  ");
    double e1 = max_err_atan(0.0, 1.0, 2000);

    remez_params_t rp = {0};
    rp.f = atan;
    rp.a = 0.0;
    rp.b = 1.0;
    rp.degree = 5;
    rp.max_iterations = 200;
    rp.tolerance = 1e-10;

    if (remez_solve(&rp) == 0) {
        printf("  Remez refinado:   ");
        double e2 = remez_max_error(&rp, 2000);
        printf("    error max: %.3e\n", e2);
        printf("    mejora:    %.1fx\n", e1 / e2);
        remez_free(&rp);
    } else {
        printf("  Remez: no convergió (%d iters)\n", rp.iterations);
    }
    printf("\n");
}

static void test_exp(void) {
    printf("=== exp(x) en [0, 1] ===\n");
    printf("  Hastings original:  ");
    double e1 = max_err_exp(0.0, 1.0, 2000);

    remez_params_t rp = {0};
    rp.f = exp;
    rp.a = 0.0;
    rp.b = 1.0;
    rp.degree = 5;
    rp.max_iterations = 200;
    rp.tolerance = 1e-10;

    if (remez_solve(&rp) == 0) {
        printf("  Remez refinado:   ");
        double e2 = remez_max_error(&rp, 2000);
        printf("    error max: %.3e\n", e2);
        printf("    mejora:    %.1fx\n", e1 / e2);
        remez_free(&rp);
    } else {
        printf("  Remez: no convergió (%d iters)\n", rp.iterations);
    }
    printf("\n");
}

static void test_log1p(void) {
    printf("=== log(1+x) en [0, 1] ===\n");
    printf("  Hastings original:  ");
    double e1 = max_err_log1p(0.0, 1.0, 2000);

    remez_params_t rp = {0};
    rp.f = log1p;
    rp.a = 0.0;
    rp.b = 1.0;
    rp.degree = 5;
    rp.max_iterations = 200;
    rp.tolerance = 1e-10;

    if (remez_solve(&rp) == 0) {
        printf("  Remez refinado:   ");
        double e2 = remez_max_error(&rp, 2000);
        printf("    error max: %.3e\n", e2);
        printf("    mejora:    %.1fx\n", e1 / e2);
        remez_free(&rp);
    } else {
        printf("  Remez: no convergió (%d iters)\n", rp.iterations);
    }
    printf("\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("  PRUEBAS DE PRECISIÓN - Hastings (1955) vs Remez (con Chebyshev)\n");
    printf("=================================================================\n\n");

    test_sin();
    test_atan();
    test_exp();
    test_log1p();

    printf("=================================================================\n");
    printf("  FIN DE PRUEBAS\n");
    printf("=================================================================\n");

    return 0;
}