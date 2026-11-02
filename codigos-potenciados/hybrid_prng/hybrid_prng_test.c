/* hybrid_prng_test.c - Test unitario rápido de hybrid_prng
 *
 * Compilar: gcc -std=c11 -O2 hybrid_prng_test.c hybrid_prng.c -o hybrid_prng_test
 * Ejecutar: ./hybrid_prng_test
 */

#include <stdio.h>
#include <stdint.h>
#include "hybrid_prng.h"

int main(void) {
    hybrid_prng_t rng;
    int failures = 0;

    /* 1. Semilla cero debe convertirse en no cero internamente */
    hybrid_prng_seed(&rng, 0ULL);
    uint64_t r1 = hybrid_prng_next(&rng);
    if (r1 == 0) {
        printf("FALLA: semilla cero produjo salida cero\n");
        failures++;
    } else {
        printf("OK: semilla cero mapeada a salida no nula: %llu\n",
               (unsigned long long)r1);
    }

    /* 2. Dos semillas distintas deben producir secuencias distintas */
    hybrid_prng_t r1_, r2_;
    hybrid_prng_seed(&r1_, 1ULL);
    hybrid_prng_seed(&r2_, 2ULL);
    int differ = 0;
    for (int i = 0; i < 100; ++i) {
        if (hybrid_prng_next(&r1_) != hybrid_prng_next(&r2_)) {
            differ = 1;
            break;
        }
    }
    if (!differ) {
        printf("FALLA: semillas distintas produjeron misma secuencia\n");
        failures++;
    } else {
        printf("OK: semillas distintas producen secuencias distintas\n");
    }

    /* 3. Mismo semilla debe reproducir la misma secuencia */
    hybrid_prng_t r3_, r4_;
    hybrid_prng_seed(&r3_, 42ULL);
    hybrid_prng_seed(&r4_, 42ULL);
    int same = 1;
    for (int i = 0; i < 100; ++i) {
        if (hybrid_prng_next(&r3_) != hybrid_prng_next(&r4_)) {
            same = 0;
            break;
        }
    }
    if (!same) {
        printf("FALLA: misma semilla produjo secuencias distintas\n");
        failures++;
    } else {
        printf("OK: misma semilla reproduce la misma secuencia\n");
    }

    /* 4. bounded debe devolver valores < n */
    hybrid_prng_t r5_;
    hybrid_prng_seed(&r5_, 7ULL);
    int bounded_ok = 1;
    for (int i = 0; i < 1000; ++i) {
        uint64_t v = hybrid_prng_next_bounded(&r5_, 17);
        if (v >= 17) { bounded_ok = 0; break; }
    }
    if (!bounded_ok) {
        printf("FALLA: bounded produjo valores fuera de rango\n");
        failures++;
    } else {
        printf("OK: bounded respeta el rango [0, n)\n");
    }

    /* 5. double en [0, 1) */
    hybrid_prng_t r6_;
    hybrid_prng_seed(&r6_, 3ULL);
    int double_ok = 1;
    for (int i = 0; i < 1000; ++i) {
        double d = hybrid_prng_next_double(&r6_);
        if (d < 0.0 || d >= 1.0) { double_ok = 0; break; }
    }
    if (!double_ok) {
        printf("FALLA: double fuera de [0,1)\n");
        failures++;
    } else {
        printf("OK: double esta en [0, 1)\n");
    }

    printf("\n%s\n", failures == 0 ? "TODOS LOS TESTS PASARON" : "ALGUNOS TESTS FALLARON");
    return failures == 0 ? 0 : 1;
}