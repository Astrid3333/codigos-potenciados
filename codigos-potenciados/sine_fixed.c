/* sine_fixed.c -- Generadores de seno en punto fijo Q16.16 / Q32.32
 *
 * Sin FPU, sin libm en camino critico: el camino de generacion es aritmetica
 * entera pura. libm se usa solo al construir las tablas y al medir el error
 * en la prueba.
 *
 * La tabla tiene un punto cada 0.01 grados (36001 puntos para el circulo
 * completo) y el muestreo tambien es de 0.01 grados, de modo que cada muestra
 * cae exactamente sobre un punto de la tabla: no hace falta interpolar ni
 * calcular angulos en coma flotante. El indice se obtiene con aritmetica
 * entera, que evita el error de truncamiento que introduce el calculo
 * (i * 2*PI / 36000) / (2*PI / 36000).
 *
 * Q16.16: valor / 65536.0      -> int32_t
 * Q32.32: valor / 4294967296.0 -> int64_t
 *
 * Las tablas se guardan en Q32.32 (int64_t) y la salida Q16.16 se deriva
 * redondeando, de modo que ambos formatos comparten la misma tabla.
 * Coste estatico: 2 tablas x 36001 x 8 bytes = 576 KB.
 *
 * Compilar: cc -std=c99 -O2 -Wall -Wextra -pedantic -DTEST sine_fixed.c -o sine_fixed -lm
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- Constantes ---------- */

#define SINE_STEPS 36000
#define SINE_TABLE_SIZE (SINE_STEPS + 1)   /* circulo completo, punto de cierre incluido */
#define Q16_ONE 65536
#define Q32_ONE 4294967296LL

/* ---------- Tablas de sincronizacion ---------- */

/* sine_table[k] = sin(k * 2*PI / 36000) * 2^32, redondeado al entero mas cercano.
 * cos_table[k] es lo mismo con el coseno. */
static int64_t sine_table[SINE_TABLE_SIZE];
static int64_t cos_table[SINE_TABLE_SIZE];

static void sine_tables_init(void) {
    int k;
    for (k = 0; k < SINE_TABLE_SIZE; ++k) {
        double theta = (double)k * (2.0 * M_PI / (double)SINE_STEPS);
        sine_table[k] = (int64_t)llround(sin(theta) * (double)Q32_ONE);
        cos_table[k] = (int64_t)llround(cos(theta) * (double)Q32_ONE);
    }
}

/* ---------- Generadores ---------- */

/* Division entera redondeada al mas cercano, simetrica respecto al cero
 * (los negativos no se sesgan hacia el cero por el truncamiento de C99). */
static int64_t div_round(int64_t num, int64_t den) {
    if (num >= 0) return (num + den / 2) / den;
    return -((-num + den / 2) / den);
}

/* Conversion Q32.32 -> Q16.16 con redondeo al entero mas cercano. */
static int32_t q32_to_q16(int64_t v) {
    return (int32_t)div_round(v, Q32_ONE / Q16_ONE);
}

/* Indice de tabla para la fase (en muestras de 0.01 grados), envuelto al circulo. */
static int sine_index(int phase) {
    int idx = phase % SINE_TABLE_SIZE;
    if (idx < 0) idx += SINE_TABLE_SIZE;
    return idx;
}

/* Escribe n muestras del seno en out[0..n-1], en Q32.32.
 * La muestra i corresponde a la fase phase_offset + i. */
void sine_q32_gen(int64_t *out, int n, int phase_offset) {
    int i;
    if (!out || n <= 0) return;
    for (i = 0; i < n; ++i) {
        out[i] = sine_table[sine_index(phase_offset + i)];
    }
}

/* Escribe n muestras del seno en out[0..n-1], en Q16.16.
 * La muestra i corresponde a la fase phase_offset + i. */
void sine_q16_gen(int32_t *out, int n, int phase_offset) {
    int i;
    if (!out || n <= 0) return;
    for (i = 0; i < n; ++i) {
        out[i] = q32_to_q16(sine_table[sine_index(phase_offset + i)]);
    }
}

/* Coseno: mismo criterio, tabla de coseno. */
void cosine_q32_gen(int64_t *out, int n, int phase_offset) {
    int i;
    if (!out || n <= 0) return;
    for (i = 0; i < n; ++i) {
        out[i] = cos_table[sine_index(phase_offset + i)];
    }
}

/* Coseno en Q16.16 */
void cosine_q16_gen(int32_t *out, int n, int phase_offset) {
    int i;
    if (!out || n <= 0) return;
    for (i = 0; i < n; ++i) {
        out[i] = q32_to_q16(cos_table[sine_index(phase_offset + i)]);
    }
}

#ifdef TEST

/* Piso teorico del formato: medio LSB. */
#define Q16_ERR_THRESHOLD (1.0 / Q16_ONE)
#define Q32_ERR_THRESHOLD (1.0 / (double)Q32_ONE)

int main(void) {
    const int n = SINE_STEPS + 1;
    static int32_t buf16[SINE_TABLE_SIZE];
    static int64_t buf32[SINE_TABLE_SIZE];
    static int32_t cbuf16[SINE_TABLE_SIZE];
    static int64_t cbuf32[SINE_TABLE_SIZE];
    double max_err16 = 0.0, max_err32 = 0.0;
    double max_err_cos16 = 0.0, max_err_cos32 = 0.0;
    int i, ok = 1;

    sine_tables_init();

    sine_q16_gen(buf16, n, 0);
    sine_q32_gen(buf32, n, 0);
    cosine_q16_gen(cbuf16, n, 0);
    cosine_q32_gen(cbuf32, n, 0);

    for (i = 0; i < n; ++i) {
        double th = (double)i * 2.0 * M_PI / (double)SINE_STEPS;
        double e16 = fabs((double)buf16[i] / (double)Q16_ONE - sin(th));
        double e32 = fabs((double)buf32[i] / (double)Q32_ONE - sin(th));
        double ec16 = fabs((double)cbuf16[i] / (double)Q16_ONE - cos(th));
        double ec32 = fabs((double)cbuf32[i] / (double)Q32_ONE - cos(th));
        if (e16 > max_err16) max_err16 = e16;
        if (e32 > max_err32) max_err32 = e32;
        if (ec16 > max_err_cos16) max_err_cos16 = ec16;
        if (ec32 > max_err_cos32) max_err_cos32 = ec32;
    }

    printf("Q16.16 max_err (seno)   = %.3e (piso %.3e)\n", max_err16, Q16_ERR_THRESHOLD);
    printf("Q16.16 max_err (coseno) = %.3e\n", max_err_cos16);
    printf("Q32.32 max_err (seno)   = %.3e (piso %.3e)\n", max_err32, Q32_ERR_THRESHOLD);
    printf("Q32.32 max_err (coseno) = %.3e\n", max_err_cos32);

    if (max_err16 > Q16_ERR_THRESHOLD) {
        printf("FALLO  error Q16.16 %.3e supera el piso %.3e\n", max_err16, Q16_ERR_THRESHOLD);
        ok = 0;
    }
    if (max_err32 > Q32_ERR_THRESHOLD) {
        printf("FALLO  error Q32.32 %.3e supera el piso %.3e\n", max_err32, Q32_ERR_THRESHOLD);
        ok = 0;
    }
    if (max_err_cos16 > Q16_ERR_THRESHOLD) {
        printf("FALLO  error de coseno Q16.16 %.3e supera el piso %.3e\n",
               max_err_cos16, Q16_ERR_THRESHOLD);
        ok = 0;
    }
    if (max_err_cos32 > Q32_ERR_THRESHOLD) {
        printf("FALLO  error de coseno Q32.32 %.3e supera el piso %.3e\n",
               max_err_cos32, Q32_ERR_THRESHOLD);
        ok = 0;
    }

    /* Continuidad del circulo: la ultima muestra debe coincidir con la primera */
    if (buf16[n - 1] != buf16[0]) {
        printf("FALLO  el circulo no cierra: %d != %d\n", buf16[n - 1], buf16[0]);
        ok = 0;
    }

    /* Pico: el seno debe alcanzar 1.0 en el cuarto de vuelta */
    {
        double peak = (double)buf16[SINE_STEPS / 4] / (double)Q16_ONE;
        if (fabs(peak - 1.0) > Q16_ERR_THRESHOLD * 4.0) {
            printf("FALLO  pico %.9f fuera de tolerancia\n", peak);
            ok = 0;
        }
    }

    if (ok) {
        printf("OK  todas las comprobaciones pasan\n");
        return EXIT_SUCCESS;
    }
    printf("FALLO  la prueba del seno no pasa\n");
    return EXIT_FAILURE;
}
#endif
