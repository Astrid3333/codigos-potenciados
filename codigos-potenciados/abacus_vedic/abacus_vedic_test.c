/* abacus_vedic_test.c - Pruebas del modulo abacus_vedic
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra abacus_vedic.c abacus_vedic_test.c -lm -o prueba
 * Ejecutar: ./prueba
 */

#include "abacus_vedic.h"
#include <stdio.h>
#include <inttypes.h>

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

static void verificar_entero(const char *nombre, unsigned long long valor,
                             unsigned long long esperado) {
    verificar(nombre, valor == esperado);
}

int main(void) {
    suanpan_estado_t a, b, r;
    int cielo[24];
    int tierra[24];
    unsigned long long cruces[40];
    unsigned long long filas[9];

    printf("=================================================================\n");
    printf("  PRUEBAS - ABACUS (SUANPAN) Y MULTIPLICACION VEDIC\n");
    printf("=================================================================\n\n");

    printf("--- Representacion de las cuentas ---\n");
    suanpan_desde_decimal(&a, 0ULL);
    verificar("el cero usa un solo poste", a.digitos == 1);
    verificar("el cero no tiene cuentas", a.cielo[0] == 0 && a.tierra[0] == 0);

    suanpan_desde_decimal(&a, 7ULL);
    verificar_entero("ida y vuelta 7", suanpan_a_decimal(&a), 7ULL);
    verificar("7 se representa con cielo + 2 cuentas de tierra",
              a.cielo[0] == 1 && a.tierra[0] == 2);

    suanpan_desde_decimal(&a, 5ULL);
    verificar("5 usa la cuenta de cielo", a.cielo[0] == 1 && a.tierra[0] == 0);

    suanpan_desde_decimal(&a, 4832ULL);
    verificar("4832 ocupa 4 postes", a.digitos == 4);
    verificar_entero("ida y vuelta 4832", suanpan_a_decimal(&a), 4832ULL);
    verificar("4832 es 4+8+3+2 de izquierda a derecha",
              suanpan_a_cadenas(&a, cielo, tierra, 24) == ABACUS_OK &&
              cielo[0] == 0 && tierra[0] == 4 &&
              cielo[1] == 1 && tierra[1] == 3 &&
              cielo[2] == 0 && tierra[2] == 3 &&
              cielo[3] == 0 && tierra[3] == 2);
    verificar("las cadenas se pueden releer",
              suanpan_desde_cadenas(&b, cielo, tierra, 4) == ABACUS_OK &&
              suanpan_a_decimal(&b) == 4832ULL);
    verificar("un buffer corto se rechaza",
              suanpan_a_cadenas(&a, cielo, tierra, 2) == ABACUS_ERROR_CAPACIDAD);

    suanpan_desde_decimal(&a, 9999999999999999999ULL);
    verificar_entero("ida y vuelta de 19 digitos", suanpan_a_decimal(&a), 9999999999999999999ULL);

    printf("\n--- Suma con acarreo de poste en poste ---\n");
    suanpan_desde_decimal(&a, 1234ULL);
    suanpan_desde_decimal(&b, 5678ULL);
    verificar("1234 + 5678 = 6912",
              suanpan_suma(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 6912ULL);
    suanpan_desde_decimal(&a, 999ULL);
    suanpan_desde_decimal(&b, 1ULL);
    verificar("999 + 1 = 1000 (acarreo encadenado)",
              suanpan_suma(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 1000ULL);
    suanpan_desde_decimal(&a, 0ULL);
    suanpan_desde_decimal(&b, 0ULL);
    verificar("0 + 0 = 0", suanpan_suma(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 0ULL);

    printf("\n--- Resta con prestamo ---\n");
    suanpan_desde_decimal(&a, 6912ULL);
    suanpan_desde_decimal(&b, 5678ULL);
    verificar("6912 - 5678 = 1234",
              suanpan_resta(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 1234ULL);
    suanpan_desde_decimal(&a, 1000ULL);
    suanpan_desde_decimal(&b, 1ULL);
    verificar("1000 - 1 = 999",
              suanpan_resta(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 999ULL);
    suanpan_desde_decimal(&a, 10ULL);
    suanpan_desde_decimal(&b, 99ULL);
    verificar("restar con minuendo menor se rechaza",
              suanpan_resta(&r, &a, &b) == ABACUS_ERROR_ARGUMENTO);

    printf("\n--- Multiplicacion por un digito ---\n");
    suanpan_desde_decimal(&a, 1234ULL);
    verificar("1234 x 7 = 8638",
              suanpan_multiplica_digito(&r, &a, 7) == ABACUS_OK && suanpan_a_decimal(&r) == 8638ULL);
    suanpan_desde_decimal(&a, 999ULL);
    verificar("999 x 9 = 8991",
              suanpan_multiplica_digito(&r, &a, 9) == ABACUS_OK && suanpan_a_decimal(&r) == 8991ULL);
    verificar("un digito fuera de rango se rechaza",
              suanpan_multiplica_digito(&r, &a, 10) == ABACUS_ERROR_ARGUMENTO);

    printf("\n--- Multiplicacion vedic (urdhva) ---\n");
    suanpan_desde_decimal(&a, 1234ULL);
    suanpan_desde_decimal(&b, 5678ULL);
    verificar("1234 x 5678 = 7006652",
              vedic_multiplicar(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 7006652ULL);
    suanpan_desde_decimal(&a, 98ULL);
    suanpan_desde_decimal(&b, 76ULL);
    verificar("98 x 76 = 7448",
              vedic_multiplicar(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 7448ULL);
    suanpan_desde_decimal(&a, 0ULL);
    suanpan_desde_decimal(&b, 12345ULL);
    verificar("0 x 12345 = 0",
              vedic_multiplicar(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 0ULL);
    suanpan_desde_decimal(&a, 7ULL);
    suanpan_desde_decimal(&b, 6ULL);
    verificar("7 x 6 = 42",
              vedic_multiplicar(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 42ULL);

    printf("\n--- Productos cruzados (convolucion) ---\n");
    suanpan_desde_decimal(&a, 123ULL);
    suanpan_desde_decimal(&b, 45ULL);
    verificar("123 x 45: cruces = 15, 22, 13, 4",
              vedic_cruces(&a, &b, cruces, 40) == ABACUS_OK &&
              cruces[0] == 15ULL && cruces[1] == 22ULL && cruces[2] == 13ULL && cruces[3] == 4ULL);
    verificar("un buffer corto se rechaza",
              vedic_cruces(&a, &b, cruces, 2) == ABACUS_ERROR_CAPACIDAD);

    printf("\n--- Tabla vedic ---\n");
    verificar("tabla del 7: 7, 14, ..., 63",
              vedic_tabla(7ULL, filas) == ABACUS_OK &&
              filas[0] == 7ULL && filas[4] == 35ULL && filas[8] == 63ULL);

    printf("\n--- Equivalencia con la convencion de las herramientas Octave ---\n");
    suanpan_desde_decimal(&a, 1234ULL);
    suanpan_desde_decimal(&b, 5678ULL);
    verificar("el total coincide con la suma moderna",
              suanpan_suma(&r, &a, &b) == ABACUS_OK && suanpan_a_decimal(&r) == 1234ULL + 5678ULL);
    suanpan_desde_decimal(&a, 4832ULL);
    suanpan_a_cadenas(&a, cielo, tierra, 24);
    verificar("4832 -> cielo [0,1,0,0] y tierra [4,3,3,2]",
              cielo[0] == 0 && cielo[1] == 1 && cielo[2] == 0 && cielo[3] == 0 &&
              tierra[0] == 4 && tierra[1] == 3 && tierra[2] == 3 && tierra[3] == 2);

    printf("\n=================================================================\n");
    printf("  %d/%d pruebas pasaron\n", pruebas_aprobadas, pruebas_totales);
    printf("=================================================================\n");

    return (pruebas_aprobadas == pruebas_totales) ? 0 : 1;
}
