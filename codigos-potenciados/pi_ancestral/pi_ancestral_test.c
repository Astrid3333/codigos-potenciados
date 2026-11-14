/* pi_ancestral_test.c - Pruebas del modulo pi_ancestral
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra pi_ancestral.c pi_ancestral_test.c -lm -o prueba
 * Ejecutar: ./prueba
 */

#include "pi_ancestral.h"

#include <math.h>
#include <stdio.h>

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
        printf("  [OK]  %s: %.15f (ref=%.15f, err=%.2e)\n", nombre, valor, esperado, error);
    } else {
        printf("  [FAIL] %s: %.15f (ref=%.15f, err=%.2e > tolerancia %.2e)\n",
               nombre, valor, esperado, error, tolerancia);
    }
}

int main(void) {
    double inferior = 0.0, superior = 0.0;
    pi_arquimedes_paso_t pasos[32];
    int i, escritos;
    double error_estable, error_ingenuo, error_liu;

    printf("=================================================================\n");
    printf("  PRUEBAS - PI ANCESTRAL (Arquimedes / Liu Hui)\n");
    printf("=================================================================\n\n");

    printf("--- Perimetro del poligono circunscrito ---\n");

    verificar_doble("hexagono circunscrito (k=0)", pi_arquimedes_estable(0),
                     2.0 * sqrt(3.0), 1e-15);
    verificar_doble("dodecagono circunscrito (k=1)", pi_arquimedes_estable(1),
                     12.0 * tan(M_PI / 12.0), 1e-14);
    verificar("k=0 todavia por encima de pi", pi_arquimedes_estable(0) > M_PI);
    verificar("k=0 por encima de pi con margen", pi_arquimedes_estable(0) - M_PI > 0.3);
    verificar("k=20 muy cerca de pi", fabs(pi_arquimedes_estable(20) - M_PI) < 1e-5);
    verificar("k=40 extremely cerca de pi", fabs(pi_arquimedes_estable(40) - M_PI) < 1e-11);

    printf("\n--- Convergencia geometrica (error ~ 4^-k) ---\n");

    error_estable = pi_arquimedes_estable(4) - M_PI;
    error_liu = fabs(pi_liu_hui(4, 1) - M_PI);
    verificar("un refinamiento de Liu Hui mejora el error",
              error_liu < error_estable);
    verificar("el error de Liu Hui cae al menos un factor 10",
              error_estable / error_liu > 10.0);

    printf("\n--- Extrapolacion de Liu Hui ---\n");

    verificar_doble("Liu Hui k=0 (sin extrapolar)", pi_liu_hui(0, 0),
                     2.0 * sqrt(3.0), 1e-15);
    verificar("Liu Hui converge a pi", fabs(pi_liu_hui(20, 3) - M_PI) < 1e-14);
    verificar("Liu Hui mejora frente al perimetro crudo",
              fabs(pi_liu_hui(20, 3) - M_PI) < fabs(pi_arquimedes_estable(20) - M_PI));
    verificar("mas niveles no empeoran el resultado",
              fabs(pi_liu_hui(20, 4) - M_PI) <= fabs(pi_liu_hui(20, 3) - M_PI) + 1e-18);

    printf("\n--- La recursion ingenua se rompe (cancelacion catastrofica) ---\n");

    error_ingenuo = fabs(pi_arquimedes_ingenuo(20) - M_PI);
    error_estable = fabs(pi_arquimedes_estable(20) - M_PI);
    verificar("k=20 la recursion ingenua ya es peor que la estable",
              error_ingenuo > error_estable);
    verificar("k=30 la recursion ingenua colapsa (0 o NaN)",
              isnan(pi_arquimedes_ingenuo(30)) || pi_arquimedes_ingenuo(30) == 0.0);
    verificar("k=60 la recursion estable sigue siendo finita",
              isfinite(pi_arquimedes_estable(60)));
    verificar("k=60 la recursion estable sigue siendo precisa",
              fabs(pi_arquimedes_estable(60) - M_PI) < 1e-14);

    printf("\n--- Cotas de Arquimedes (media armonica / geometrica) ---\n");

    pi_arquimedes_cotas(0, &inferior, &superior);
    verificar_doble("cota inferior k=0 (hexagono inscrito)", inferior, 3.0, 1e-15);
    verificar_doble("cota superior k=0 (hexagono circunscrito)", superior, 2.0 * sqrt(3.0), 1e-15);
    verificar("las cotas envuelven a pi en k=0", inferior < M_PI && M_PI < superior);

    pi_arquimedes_cotas(10, &inferior, &superior);
    verificar("las cotas envuelven a pi en k=10", inferior < M_PI && M_PI < superior);
    verificar("la cota inferior de k=10 supera a la de k=0", inferior > 3.0);
    verificar("la cota superior de k=10 baja del hexagono", superior < 2.0 * sqrt(3.0));
    verificar("el hueco entre cotas es pequeño en k=10", superior - inferior < 1e-6);

    printf("\n--- Tabla historica ---\n");

    escritos = pi_arquimedes_pasos(30, pasos, 32);
    verificar("la tabla registra 31 pasos", escritos == 31);
    verificar("el primer paso tiene 6 lados", pasos[0].lados == 6);
    verificar("los lados se duplican", pasos[1].lados == 12 && pasos[5].lados == 192);
    verificar("el perimetro decrece en la tabla",
              pasos[0].perimetro > pasos[1].perimetro && pasos[4].perimetro > pasos[5].perimetro);
    verificar("las cotas de la tabla envuelven a pi",
              pasos[5].cota_inferior < M_PI && M_PI < pasos[5].cota_superior);
    verificar("la tabla ingenua ya diverge en los ultimos pasos",
              isnan(pasos[30].perimetro_ingenuo) || pasos[30].perimetro_ingenuo == 0.0);
    verificar("la tabla ingenua es peor que la estable en el paso 20",
              fabs(pasos[20].perimetro_ingenuo - M_PI) >
              fabs(pasos[20].perimetro - M_PI));

    printf("\n--- Muestra de la tabla ---\n");
    printf("  %6s %20s %20s %20s\n", "lados", "perimetro", "cota inf", "cota sup");
    for (i = 0; i < escritos; ++i) {
        printf("  %6ld %20.15f %20.15f %20.15f\n",
               pasos[i].lados, pasos[i].perimetro,
               pasos[i].cota_inferior, pasos[i].cota_superior);
    }

    printf("\n=================================================================\n");
    printf("  %d/%d pruebas pasaron\n", pruebas_aprobadas, pruebas_totales);
    printf("=================================================================\n");

    return (pruebas_aprobadas == pruebas_totales) ? 0 : 1;
}
