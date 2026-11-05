/* matematica_ancestral_test.c - Pruebas del módulo matemática ancestral
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra matematica_ancestral.c matematica_ancestral_test.c -lm -o prueba
 * Ejecutar: ./prueba
 */

#include "matematica_ancestral.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <inttypes.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int pruebas_totales = 0;
static int pruebas_aprobadas = 0;

static void verificar(const char *nombre, int aprobado) {
    pruebas_totales++;
    if (aprobado) {
        pruebas_aprobadas++;
        printf("  [OK]  %s\n", nombre);
    } else {
        printf("  [FAIL] %s\n", nombre);
    }
}

static void verificar_doble(const char *nombre, double valor, double esperado, double tolerancia) {
    double error = fabs(valor - esperado);
    int aprobado = error <= tolerancia;
    pruebas_totales++;
    if (aprobado) {
        pruebas_aprobadas++;
        printf("  [OK]  %s: %.6f (ref=%.6f, err=%.2e)\n", nombre, valor, error);
    } else {
        printf("  [FAIL] %s: %.6f (ref=%.6f, err=%.2e > tolerancia %.2e)\n",
               nombre, valor, esperado, error, tolerancia);
    }
}

int main(void) {
    printf("=================================================================\n");
    printf("  PRUEBAS - MATEMÁTICA ANCESTRAL\n");
    printf("=================================================================\n\n");

    /* ============================================================ */
    /* 1. Multiplicación egipcia                                   */
    /* ============================================================ */
    printf("\n--- Multiplicación egipcia (duplicación y adición) ---\n");

    verificar("6 * 7 = 42", multiplicacion_egipcia(6, 7) == 42);
    verificar("13 * 12 = 156", multiplicacion_egipcia(13, 12) == 156);
    verificar("100 * 1 = 100", multiplicacion_egipcia(100, 1) == 100);
    verificar("0 * 123 = 0", multiplicacion_egipcia(0, 123) == 0);
    verificar("(-3) * 7 = -21", multiplicacion_egipcia(-3, 7) == -21);
    verificar("(-3) * (-7) = 21", multiplicacion_egipcia(-3, -7) == 21);
    verificar("25 * 25 = 625", multiplicacion_egipcia(25, 25) == 625);

    /* ============================================================ */
    /* 2. Fracciones egipcias                                      */
    /* ============================================================ */
    printf("\n--- Fracciones egipcias (descomposición en fracciones unitarias) ---\n");

    {
        int64_t den[10];
        int n = fracciones_egipcias(2, 3, den, 10);
        /* 2/3 = 1/2 + 1/6 */
        int aprobado = (n == 2 && den[0] == 2 && den[1] == 6);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  2/3 = 1/2 + 1/6\n");
        } else {
            printf("  [FAIL] 2/3 = ");
            for (int i = 0; i < n; i++) {
                printf("1/%" PRId64, den[i]);
                if (i < n - 1) printf(" + ");
            }
            printf(" (n=%d)\n", n);
        }
    }

    {
        int64_t den[10];
        int n = fracciones_egipcias(3, 7, den, 10);
        /* 3/7 = 1/3 + 1/11 + 1/231 */
        int aprobado = (n == 3 && den[0] == 3 && den[1] == 11 && den[2] == 231);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  3/7 = 1/3 + 1/11 + 1/231\n");
        } else {
            printf("  [FAIL] 3/7 = ");
            for (int i = 0; i < n; i++) {
                printf("1/%" PRId64, den[i]);
                if (i < n - 1) printf(" + ");
            }
            printf(" (n=%d)\n", n);
        }
    }

    /* ============================================================ */
    /* 3. Raíz cuadrada babilónica                                 */
    /* ============================================================ */
    printf("\n--- Raíz cuadrada babilónica (método de Heron) ---\n");

    verificar_doble("sqrt(2)", raiz_cuadrada_babilonia(2.0, 10), 1.41421356, 1e-6);
    verificar_doble("sqrt(9)", raiz_cuadrada_babilonia(9.0, 10), 3.0, 1e-6);
    verificar_doble("sqrt(100)", raiz_cuadrada_babilonia(100.0, 10), 10.0, 1e-6);
    verificar_doble("sqrt(0.25)", raiz_cuadrada_babilonia(0.25, 10), 0.5, 1e-6);
    verificar_doble("sqrt(2)", raiz_cuadrada_babilonia(2.0, 100), 1.41421356237, 1e-10);

    /* ============================================================ */
    /* 4. Seno de Aryabhata                                        */
    /* ============================================================ */
    printf("\n--- Seno de Aryabhata (India, 499 d.C.) ---\n");

    {
        double ref[] = {0.0, 0.5, 0.8660254, 1.0, 0.8660254, 0.5, 0.0};
        double angulos[] = {0.0, M_PI/6, M_PI/3, M_PI/2, 2*M_PI/3, 5*M_PI/6, M_PI};
        int n = sizeof(angulos) / sizeof(angulos[0]);

        for (int i = 0; i < n; i++) {
            double resultado = seno_aryabhata(angulos[i], 360);
            double error = fabs(resultado - ref[i]);
            int aprobado = error < 2e-2;
            pruebas_totales++;
            if (aprobado) {
                pruebas_aprobadas++;
                printf("  [OK]  sin(%.0f°) = %.6f (ref=%.6f, err=%.2e)\n",
                       angulos[i] * 180.0 / M_PI, resultado, ref[i], error);
            } else {
                printf("  [FAIL] sin(%.0f°) = %.6f (ref=%.6f, err=%.2e)\n",
                       angulos[i] * 180.0 / M_PI, resultado, ref[i], error);
            }
        }
    }

    /* ============================================================ */
    /* 5. Teorema Chino del Resto                                  */
    /* ============================================================ */
    printf("\n--- Teorema Chino del Resto (China antigua) ---\n");

    {
        int64_t residuos[] = {2, 3, 2};
        int64_t modulos[] = {3, 5, 7};
        int64_t M;
        int64_t resultado = teorema_chino_resto(residuos, modulos, 3, &M);
        /* x ≡ 2 (mod 3), x ≡ 3 (mod 5), x ≡ 2 (mod 7) => x = 23 */
        int aprobado = (resultado == 23);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  x ≡ 2(mod 3), x ≡ 3(mod 5), x ≡ 2(mod 7) => x = %lld\n", (long long)resultado);
        } else {
            printf("  [FAIL] x ≡ 2(mod 3), x ≡ 3(mod 5), x ≡ 2(mod 7) => x = %lld (esperado 23)\n", (long long)resultado);
        }
    }

    {
        int64_t residuos[] = {1, 2, 3};
        int64_t modulos[] = {5, 7, 11};
        int64_t M;
        int64_t resultado = teorema_chino_resto(residuos, modulos, 3, &M);
        /* x ≡ 1 (mod 5), x ≡ 2 (mod 7), x ≡ 3 (mod 11) => x = 366 */
        int aprobado = (resultado == 366);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  x ≡ 1(mod 5), x ≡ 2(mod 7), x ≡ 3(mod 11) => x = %lld\n", (long long)resultado);
        } else {
            printf("  [FAIL] x ≡ 1(mod 5), x ≡ 2(mod 7), x ≡ 3(mod 11) => x = %lld (esperado 366)\n", (long long)resultado);
        }
    }

    /* ============================================================ */
    /* 6. Sistema vigesimal maya                                   */
    /* ============================================================ */
    printf("\n--- Sistema vigesimal maya (base 20) ---\n");

    {
        int digitos[10];
        int n = sistema_vigesimal_maya(0, digitos, 10);
        int aprobado = (n == 1 && digitos[0] == 0);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  0 = [0]\n");
        } else {
            printf("  [FAIL] 0 = ");
            for (int i = 0; i < n; i++) printf("%d ", digitos[i]);
            printf("(n=%d)\n", n);
        }
    }

    {
        int digitos[10];
        int n = sistema_vigesimal_maya(20, digitos, 10);
        /* 20 = 1*20 + 0 => digitos = [0, 1] */
        int aprobado = (n == 2 && digitos[0] == 0 && digitos[1] == 1);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  20 = [0, 1] (1*20 + 0)\n");
        } else {
            printf("  [FAIL] 20 = ");
            for (int i = 0; i < n; i++) printf("%d ", digitos[i]);
            printf("(n=%d)\n", n);
        }
    }

    {
        int digitos[10];
        int n = sistema_vigesimal_maya(399, digitos, 10);
        /* 399 = 19*20 + 19 => digitos = [19, 19] */
        int aprobado = (n == 2 && digitos[0] == 19 && digitos[1] == 19);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  399 = [19, 19] (19*20 + 19)\n");
        } else {
            printf("  [FAIL] 399 = ");
            for (int i = 0; i < n; i++) printf("%d ", digitos[i]);
            printf("(n=%d)\n", n);
        }
    }

    /* ============================================================ */
    /* 7. Eliminación china                                         */
    /* ============================================================ */
    printf("\n--- Eliminación china (método de los nueve capítulos) ---\n");

    {
        /* Sistema:
         * 2x + y = 5
         *  x + 3y = 5
         * Solución: x = 2, y = 1
         */
        double A[2][2] = {{2, 1}, {1, 3}};
        double b[2] = {5, 5};
        double x[2];

        double *A_ptr[2] = {A[0], A[1]};
        int resultado = eliminacion_china(A_ptr, b, x, 2);
        int aprobado = (resultado == 0 && fabs(x[0] - 2.0) < 1e-10 && fabs(x[1] - 1.0) < 1e-10);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  2x + y = 5, x + 3y = 5 => x=%.6f, y=%.6f (esperado x=2, y=1)\n", x[0], x[1]);
        } else {
            printf("  [FAIL] 2x + y = 5, x + 3y = 5 => x=%.6f, y=%.6f (esperado x=2, y=1)\n", x[0], x[1]);
        }
    }

    {
        /* Sistema:
         *  x +  y = 3
         * 2x +  y = 4
         * Solución: x = 1, y = 2
         */
        double A[2][2] = {{1, 1}, {2, 1}};
        double b[2] = {3, 4};
        double x[2];

        double *A_ptr[2] = {A[0], A[1]};
        int resultado = eliminacion_china(A_ptr, b, x, 2);
        int aprobado = (resultado == 0 && fabs(x[0] - 1.0) < 1e-10 && fabs(x[1] - 2.0) < 1e-10);
        pruebas_totales++;
        if (aprobado) {
            pruebas_aprobadas++;
            printf("  [OK]  x + y = 3, 2x + y = 4 => x=%.6f, y=%.6f (esperado x=1, y=2)\n", x[0], x[1]);
        } else {
            printf("  [FAIL] x + y = 3, 2x + y = 4 => x=%.6f, y=%.6f (esperado x=1, y=2)\n", x[0], x[1]);
        }
    }

    /* ============================================================ */
    /* RESUMEN                                                      */
    /* ============================================================ */
    printf("\n=================================================================\n");
    printf("  RESULTADO: %d/%d pruebas aprobadas\n", pruebas_aprobadas, pruebas_totales);
    if (pruebas_aprobadas == pruebas_totales) {
        printf("  TODAS LAS PRUEBAS PASARON\n");
        return 0;
    } else {
        printf("  ALGUNAS PRUEBAS FALLARON\n");
        return 1;
    }
}