/* cordic_test.c - Pruebas del algoritmo CORDIC
 *
 * Compara los resultados de CORDIC contra las funciones de libm
 * (sin, cos) para verificar precisión en diferentes rangos de ángulo.
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra cordic.c cordic_test.c -lm -o cordic_test
 * Ejecutar: ./cordic_test
 */

#include "cordic.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double max_abs_err_sin = 0.0;
static double max_abs_err_cos = 0.0;
static double max_rel_err_sin = 0.0;
static double max_rel_err_cos = 0.0;
static int n_tests = 0;

static void check_point(double theta_deg) {
    double theta = theta_deg * M_PI / 180.0;

    double ref_sin = sin(theta);
    double ref_cos = cos(theta);

    cordic_result_t r = cordic_sincos(theta);
    double c = r.x / CORDIC_K;
    double s = r.y / CORDIC_K;

    double err_sin = fabs(s - ref_sin);
    double err_cos = fabs(c - ref_cos);

    if (fabs(ref_sin) > 1e-12) {
        double rel_sin = err_sin / fabs(ref_sin);
        if (rel_sin > max_rel_err_sin) max_rel_err_sin = rel_sin;
    }
    if (fabs(ref_cos) > 1e-12) {
        double rel_cos = err_cos / fabs(ref_cos);
        if (rel_cos > max_rel_err_cos) max_rel_err_cos = rel_cos;
    }

    if (err_sin > max_abs_err_sin) max_abs_err_sin = err_sin;
    if (err_cos > max_abs_err_cos) max_abs_err_cos = err_cos;

    n_tests++;
}

static void test_range(double start_deg, double end_deg, double step_deg) {
    double theta;
    for (theta = start_deg; theta <= end_deg + 1e-9; theta += step_deg) {
        check_point(theta);
    }
}

int main(void) {
    printf("=================================================================\n");
    printf("  PRUEBAS DE PRECISIÓN - CORDIC (1959) vs libm\n");
    printf("=================================================================\n\n");

    /* Barrido completo: -180° a +180° en pasos de 0.5° */
    printf("Probando rango completo [-180°, +180°] en pasos de 0.5°...\n");
    test_range(-180.0, 180.0, 0.5);

    printf("\nResultados (iteraciones = %d):\n", CORDIC_ITERATIONS);
    printf("  error absoluto máximo sin:  %.3e\n", max_abs_err_sin);
    printf("  error absoluto máximo cos:  %.3e\n", max_abs_err_cos);
    printf("  error relativo máximo sin:  %.3e\n", max_rel_err_sin);
    printf("  error relativo máximo cos:  %.3e\n", max_rel_err_cos);
    printf("  puntos probados:            %d\n", n_tests);

    /* Pruebas de ángulos conocidos */
    printf("\nPruebas de ángulos conocidos:\n");
    {
        struct { double deg; const char *name; } tests[] = {
            {0.0,    "0°"},
            {30.0,   "30°"},
            {45.0,   "45°"},
            {60.0,   "60°"},
            {90.0,   "90°"},
            {180.0,  "180°"},
            {270.0,  "270°"},
            {-45.0,  "-45°"},
            {-90.0,  "-90°"},
        };
        int n = sizeof(tests) / sizeof(tests[0]);
        int i;

        for (i = 0; i < n; i++) {
            double theta = tests[i].deg * M_PI / 180.0;
            cordic_result_t r = cordic_sincos(theta);
            double c = r.x / CORDIC_K;
            double s = r.y / CORDIC_K;
            double ref_s = sin(theta);
            double ref_c = cos(theta);

            printf("  %-6s: sin=%.10f (ref=%.10f, err=%.2e)  "
                   "cos=%.10f (ref=%.10f, err=%.2e)\n",
                   tests[i].name, s, ref_s, fabs(s - ref_s),
                   c, ref_c, fabs(c - ref_c));
        }
    }

    printf("\n=================================================================\n");
    if (max_abs_err_sin < 5e-5 && max_abs_err_cos < 5e-5) {
        printf("  RESULTADO: ACEPTABLE (error < 5e-5)\n");
        return 0;
    } else {
        printf("  RESULTADO: NECESITA MÁS ITERACIONES\n");
        return 1;
    }
}