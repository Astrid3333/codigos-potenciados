/* randu_plus.c - Implementación y test mínimo
 *
 * Compilar: gcc -std=c11 -O2 randu_plus.c -o randu_test
 * Ejecutar: ./randu_test
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "randu_plus.h"

int main(void) {
    uint32_t s = 1;  // semilla no nula
    printf("Prueba RANDU+fmix32 (primeros 10):\n");
    for (int i = 0; i < 10; ++i) {
        uint32_t r = randu_plus(&s);
        printf("%2d: %10" PRIu32 "  (0x%08" PRIX32 ")\n", i, r, r);
    }

    // Test de período básico: RANDU puro repite a 2^31 / 2^2 = 2^29 aprox
    // con el mezclador no cambia el período del estado interno
    printf("\nEstado final: %" PRIu32 "\n", s);
    return 0;
}