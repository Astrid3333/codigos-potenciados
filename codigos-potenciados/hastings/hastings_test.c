/* hastings_test.c - Pruebas de precisión para Hastings (1955) y Remez
 *
 * Compara las aproximaciones de Hastings con las refinadas por el algoritmo
 * de intercambio de Remez, midiendo el error máximo contra libm, y verifica
 * la corrección del signo de sin en toda la circunferencia.
 *
 * Compilar: gcc -std=c11 -O2 hastings.c hastings_test.c -o hastings_test -lm
 * Ejecutar: ./hastings_test
 */

#include "hastings.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int tests_run = 0;
static int tests_failed = 0;

static void check(const char *name, int ok, const char *detail) {
    tests_run++;
    if (!ok) tests_failed++;
    printf("  [%s] %s%s%s\n", ok ? "OK" : "FAIL", name,
           detail ? "  " : "", detail ? detail : "");
}

/* ---------- Funciones auxiliares de error máximo ---------- */

static double max_err_sin(double a, double b, int n, double *max_x) {
    double max_err = 0.0;
    *max_x = a;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double err = fabs(sin(x) - hastings_sin(x).value);
        if (err > max_err) { max_err = err; *max_x = x; }
    }
    return max_err;
}

static double max_err_atan(double a, double b, int n) {
    double max_err = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double err = fabs(atan(x) - hastings_atan(x).value);
        if (err > max_err) max_err = err;
    }
    return max_err;
}

static double max_rel_err_exp(double a, double b, int n) {
    double max_err = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double err = fabs(exp(x) - hastings_exp(x).value) / exp(x);
        if (err > max_err) max_err = err;
    }
    return max_err;
}

static double max_err_log1p(double a, double b, int n) {
    double max_err = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double err = fabs(log1p(x) - hastings_log1p(x).value);
        if (err > max_err) max_err = err;
    }
    return max_err;
}

/* ---------- Pruebas de sin ---------- */

static void test_sin(void) {
    printf("=== sin(x) ===\n");
    double at_x = 0.0;
    char buf[160];

    double e_quarter = max_err_sin(0.0, M_PI / 2.0, 2000, &at_x);
    snprintf(buf, sizeof buf, "error max en [0,pi/2] = %.3e (x=%.6f)", e_quarter, at_x);
    check("sin error max en [0,pi/2] <= 1e-6", e_quarter <= 1e-6, buf);

    /* Regresion: la reduccion a [0,pi/2] debe conservar el signo. */
    double e_full = max_err_sin(0.0, 2.0 * M_PI, 4000, &at_x);
    snprintf(buf, sizeof buf, "error max en [0,2pi] = %.3e (x=%.6f)", e_full, at_x);
    check("sin error max en [0,2pi] <= 1e-6", e_full <= 1e-6, buf);

    double e_neg = max_err_sin(-2.0 * M_PI, 0.0, 4000, &at_x);
    snprintf(buf, sizeof buf, "error max en [-2pi,0] = %.3e", e_neg);
    check("sin error max en [-2pi,0] <= 1e-6", e_neg <= 1e-6, buf);

    /* Valores con signo en puntos clave. */
    const double pts[] = {M_PI / 6.0, M_PI / 2.0, 2.0 * M_PI / 3.0, M_PI,
                          3.0 * M_PI / 2.0, 5.0 * M_PI / 4.0, 2.0 * M_PI};
    double worst = 0.0;
    for (size_t i = 0; i < sizeof pts / sizeof pts[0]; i++) {
        double err = fabs(sin(pts[i]) - hastings_sin(pts[i]).value);
        if (err > worst) worst = err;
    }
    snprintf(buf, sizeof buf, "error max en puntos clave = %.3e", worst);
    check("sin en puntos clave (incluye 2pi/3 y 3pi/2) <= 1e-6", worst <= 1e-6, buf);

    /* Simetria impar: sin(-x) = -sin(x). */
    double asym = 0.0;
    for (int i = 1; i <= 1000; i++) {
        double x = 2.0 * M_PI * (double)i / 1000.0;
        double err = fabs(hastings_sin(-x).value + hastings_sin(x).value);
        if (err > asym) asym = err;
    }
    snprintf(buf, sizeof buf, "|sin(-x)+sin(x)| max = %.3e", asym);
    check("sin es impar en [0,2pi] <= 1e-6", asym <= 1e-6, buf);
    printf("\n");
}

/* ---------- Pruebas de atan, exp y log1p ---------- */

static void test_atan(void) {
    printf("=== atan(x) ===\n");
    char buf[160];
    double e = max_err_atan(0.0, 1.0, 2000);
    snprintf(buf, sizeof buf, "error max en [0,1] = %.3e", e);
    check("atan error max en [0,1] <= 1e-5", e <= 1e-5, buf);

    double e4 = max_err_atan(0.05, 4.0, 4000);
    snprintf(buf, sizeof buf, "error max en [0.05,4] = %.3e", e4);
    check("atan error max en [0.05,4] <= 1e-5", e4 <= 1e-5, buf);

    double e1 = fabs(atan(1.0) - hastings_atan(1.0).value);
    snprintf(buf, sizeof buf, "error en atan(1) = %.3e", e1);
    check("atan(1) = pi/4 con error <= 1e-12", e1 <= 1e-12, buf);
    printf("\n");
}

static void test_exp(void) {
    printf("=== exp(x) ===\n");
    char buf[160];
    double e = max_rel_err_exp(-5.0, 5.0, 4000);
    snprintf(buf, sizeof buf, "error relativo max en [-5,5] = %.3e", e);
    check("exp error relativo max en [-5,5] <= 1e-5", e <= 1e-5, buf);

    double e0 = fabs(1.0 - hastings_exp(0.0).value);
    snprintf(buf, sizeof buf, "error en exp(0) = %.3e", e0);
    check("exp(0) = 1 con error <= 1e-12", e0 <= 1e-12, buf);

    double e1 = fabs(exp(1.0) - hastings_exp(1.0).value);
    snprintf(buf, sizeof buf, "error en exp(1) = %.3e", e1);
    check("exp(1) con error <= 1e-6", e1 <= 1e-6, buf);
    printf("\n");
}

static void test_log1p(void) {
    printf("=== log1p(x) ===\n");
    char buf[160];
    double e = max_err_log1p(0.0, 1.0, 2000);
    snprintf(buf, sizeof buf, "error max en [0,1] = %.3e", e);
    check("log1p error max en [0,1] <= 1e-5", e <= 1e-5, buf);

    double e1 = fabs(log1p(1.0) - hastings_log1p(1.0).value);
    snprintf(buf, sizeof buf, "error en log1p(1) = %.3e", e1);
    check("log1p(1) = ln2 con error <= 1e-12", e1 <= 1e-12, buf);
    printf("\n");
}

/* ---------- Pruebas de Remez ---------- */

/* Polinomio de Taylor de grado 5 en x, para comparar con Remez de grado 5. */
static double taylor5(int kind, double x) {
    double x2 = x * x;
    switch (kind) {
        case 0: return x - x2 * x / 6.0 + x2 * x2 * x / 120.0;            /* sin */
        case 1: return x - x2 * x / 3.0 + x2 * x2 * x / 5.0;              /* atan */
        case 2: return 1.0 + x + x2 / 2.0 + x2 * x / 6.0 + x2 * x2 / 24.0
                    + x2 * x2 * x / 120.0;                                /* exp */
        default: return x - x2 / 2.0 + x2 * x / 3.0 - x2 * x2 / 4.0
                     + x2 * x2 * x / 5.0;                                 /* log1p */
    }
}

static double max_err_taylor5(int kind, double a, double b, int n) {
    double max_err = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double err = 0.0;
        switch (kind) {
            case 0: err = fabs(sin(x) - taylor5(kind, x)); break;
            case 1: err = fabs(atan(x) - taylor5(kind, x)); break;
            case 2: err = fabs(exp(x) - taylor5(kind, x)); break;
            default: err = fabs(log1p(x) - taylor5(kind, x)); break;
        }
        if (err > max_err) max_err = err;
    }
    return max_err;
}

/* Taylor plano (sin reduccion de argumento) del grado d pedido, para
 * comparar con Remez del mismo grado: es la unica comparacion justa, porque
 * el modulo aplica ademas reduccion de argumento y elevaciones al cuadrado. */
static double taylor_deg(int kind, double x, int deg) {
    double s = 0.0, term;
    switch (kind) {
        case 0: /* sin: x - x^3/3! + x^5/5! - ... */
            term = 1.0; s = 0.0;
            for (int k = 0; k <= (deg - 1) / 2; k++) {
                s += ((k % 2) ? -1.0 : 1.0) * term;
                term *= x * x / ((2.0 * k + 2.0) * (2.0 * k + 3.0));
            }
            return s * x;
        case 1: /* atan: x - x^3/3 + x^5/5 - ... */
            s = 0.0; term = 1.0;
            for (int k = 0; k <= (deg - 1) / 2; k++) {
                s += ((k % 2) ? -1.0 : 1.0) * term;
                term *= x * x * (2.0 * k + 1.0) / (2.0 * k + 3.0);
            }
            return s * x;
        case 2: /* exp: 1 + x + x^2/2! + ... */
            s = 0.0; term = 1.0;
            for (int k = 0; k <= deg; k++) {
                if (k > 0) term *= x / k;
                s += term;
            }
            return s;
        default: /* log1p: x - x^2/2 + x^3/3 - ... */
            s = 0.0; term = x;
            for (int k = 1; k <= deg; k++) {
                if (k > 1) term *= -x * (k - 1.0) / k;
                s += term;
            }
            return s;
    }
}

static double max_err_taylor_deg(int kind, double a, double b, int n, int deg) {
    double max_err = 0.0;
    for (int i = 0; i <= n; i++) {
        double x = a + (double)i / (double)n * (b - a);
        double truth = (kind == 0) ? sin(x) : (kind == 1) ? atan(x)
                      : (kind == 2) ? exp(x) : log1p(x);
        double err = fabs(truth - taylor_deg(kind, x, deg));
        if (err > max_err) max_err = err;
    }
    return max_err;
}

static void test_remez(void) {
    printf("=== Remez (minimax) ===\n");
    char buf[240];
    struct {
        const char *name;
        remez_func_t f;
        double a, b;
        int degree;      /* grado real del polinomio del modulo, en x */
        int kind;        /* 0 sin, 1 atan, 2 exp, 3 log1p */
        int modulo_rel;  /* 1 si el error del modulo se mide en relativo */
    } cases[] = {
        {"sin", sin, 0.0, M_PI / 2.0, 11, 0, 0},
        {"atan", atan, 0.0, 1.0, 11, 1, 0},
        {"exp", exp, 0.0, 1.0, 5, 2, 1},
        {"log1p", log1p, 0.0, 1.0, 9, 3, 0},
    };

    for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        double taylor5_err = max_err_taylor5(cases[c].kind, cases[c].a, cases[c].b, 2000);
        double hastings_err;
        if (cases[c].kind == 0) {
            double dummy = 0.0;
            hastings_err = max_err_sin(cases[c].a, cases[c].b, 2000, &dummy);
        } else if (cases[c].kind == 1) {
            hastings_err = max_err_atan(cases[c].a, cases[c].b, 2000);
        } else if (cases[c].kind == 2) {
            hastings_err = max_rel_err_exp(cases[c].a, cases[c].b, 2000);
        } else {
            hastings_err = max_err_log1p(cases[c].a, cases[c].b, 2000);
        }

        /* 1) Remez de grado 5 contra Taylor de grado 5: comparacion justa. */
        remez_params_t rp5 = {0};
        rp5.f = cases[c].f;
        rp5.a = cases[c].a;
        rp5.b = cases[c].b;
        rp5.degree = 5;
        rp5.max_iterations = 200;
        rp5.tolerance = 1e-12;
        int rc5 = remez_solve(&rp5);
        double err5 = remez_max_error(&rp5, 2000);
        snprintf(buf, sizeof buf, "%s: rc=%d iter=%d Remez(5) = %.3e, Taylor(5) = %.3e",
                 cases[c].name, rc5, rp5.iterations, err5, taylor5_err);
        check("Remez converge", rc5 == 0, buf);
        check("Remez(grado 5) mejora a Taylor(grado 5)", err5 < taylor5_err, buf);
        remez_free(&rp5);

        /* 2) Remez del grado real contra Taylor plano del MISMO grado.
         * El minimax es optimo, asi que no puede ser peor que el Taylor del
         * mismo grado. El error del modulo (con reduccion de argumento) se
         * informa aparte: comparar un polinomio simple contra una formula
         * compuesta no es una comparacion like-for-like. */
        double taylor_d_err = max_err_taylor_deg(cases[c].kind, cases[c].a,
                                                 cases[c].b, 2000, cases[c].degree);
        remez_params_t rpx = {0};
        rpx.f = cases[c].f;
        rpx.a = cases[c].a;
        rpx.b = cases[c].b;
        rpx.degree = cases[c].degree;
        rpx.max_iterations = 200;
        rpx.tolerance = 1e-12;
        int rcx = remez_solve(&rpx);
        double errx = remez_max_error(&rpx, 2000);
        snprintf(buf, sizeof buf,
                 "%s: rc=%d iter=%d Remez(%d) = %.3e, Taylor(%d) = %.3e, "
                 "modulo con reduccion%s = %.3e",
                 cases[c].name, rcx, rpx.iterations, cases[c].degree, errx,
                 cases[c].degree, taylor_d_err,
                 cases[c].modulo_rel ? " (relativo)" : "", hastings_err);
        check("Remez converge con el grado real del polinomio", rcx == 0, buf);
        check("Remez(grado real) no es peor que Taylor del mismo grado",
              errx <= taylor_d_err * 1.10 + 1e-15, buf);

        /* 3) Alternancia: el error debe cambiar de signo al menos m-1 veces. */
        int m = rpx.degree + 2;
        int signs = 0, last = 0;
        for (int i = 0; i <= 4000; i++) {
            double x = rpx.a + (rpx.b - rpx.a) * (double)i / 4000.0;
            double e = cases[c].f(x) - remez_evaluate(&rpx, x);
            int s = (e > 0.0) ? 1 : ((e < 0.0) ? -1 : 0);
            if (s != 0 && s != last) { signs++; last = s; }
        }
        snprintf(buf, sizeof buf, "%s: cambios de signo del error = %d (minimo %d)",
                 cases[c].name, signs, m - 1);
        check("Remez produce alternancia del error", signs >= m - 1, buf);
        remez_free(&rpx);
    }
    printf("\n");
}

static void test_metadatos(void) {
    printf("=== Metadatos de los polinomios ===\n");
    char buf[200];
    struct {
        const char *name;
        int n_expected;    /* numero de coeficientes */
        int deg_expected;  /* grado en x */
    } cases[] = {
        {"sin", 6, 11},
        {"atan", 6, 11},
        {"exp", 6, 5},
        {"log1p", 10, 9},
    };
    for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++) {
        const hastings_poly_t *p = hastings_get_poly(cases[c].name);
        const hastings_info_t *i = hastings_info(cases[c].name);
        snprintf(buf, sizeof buf, "%s: n=%d (esperado %d), grado=%d (esperado %d)",
                 cases[c].name, p ? p->n : -1, cases[c].n_expected,
                 i ? i->degree : -1, cases[c].deg_expected);
        check("hastings_get_poly declara el numero de coeficientes",
              p && p->n == cases[c].n_expected, buf);
        check("hastings_info declara el grado en x",
              i && i->degree == cases[c].deg_expected, buf);
    }
    snprintf(buf, sizeof buf, "hastings_sin(0.5).degree = %d (esperado 11)",
             hastings_sin(0.5).degree);
    check("hastings_sin informa el grado 11", hastings_sin(0.5).degree == 11, buf);
    snprintf(buf, sizeof buf, "hastings_exp(0.5).degree = %d (esperado 5)",
             hastings_exp(0.5).degree);
    check("hastings_exp informa el grado 5", hastings_exp(0.5).degree == 5, buf);
    snprintf(buf, sizeof buf, "hastings_log1p(0.5).degree = %d (esperado 9)",
             hastings_log1p(0.5).degree);
    check("hastings_log1p informa el grado 9", hastings_log1p(0.5).degree == 9, buf);
    printf("\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("  PRUEBAS DE PRECISIÓN - Hastings (1955) vs Remez\n");
    printf("=================================================================\n\n");

    test_sin();
    test_atan();
    test_exp();
    test_log1p();
    test_metadatos();
    test_remez();

    printf("=================================================================\n");
    printf("  %d/%d pruebas pasaron\n", tests_run - tests_failed, tests_run);
    printf("=================================================================\n");
    return tests_failed == 0 ? 0 : 1;
}
