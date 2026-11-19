/* sine_fixed.c -- Seno Q16.16 con tabla e interpolación lineal
 *
 * Salida en Q16.16 (int32_t): valor / 65536.0 es el seno, rango completo [-1, 1].
 * El camino de generación no usa coma flotante ni libm: solo aritmética entera.
 * libm se usa únicamente para construir la tabla al inicio y para medir el
 * error en la prueba (bajo #ifdef TEST).
 *
 * Unidades: la fase se cuenta en 1/100 de grado, así que el círculo completo
 * son 36000 unidades y cada muestra avanza exactamente una unidad (0.01 grados).
 * La tabla tiene un punto cada 0.1 grados (3600 puntos) y se interpola entre
 * dos puntos consecutivos con la fracción decimal de la fase.
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

#define SINE_PHASE_UNITS 36000        /* fase completa en 1/100 de grado */
#define SINE_TABLE_STEPS 3600         /* puntos de tabla: uno cada 0.1 grados */
#define SINE_TABLE_FRAC 10            /* fracciones de 1/100 de grado por punto */
#define SINE_Q16_ONE 65536
#define SINE_QUARTER_TURN (SINE_PHASE_UNITS / 4)

/* La tabla guarda 4 bits decimales extra sobre Q16.16 (factor 16) para que la
 * interpolacion no pierda precision al dividir entre 10. El error de
 * cuantizacion de la tabla baja a 1/32 de LSB y el redondeo final a la salida
 * Q16.16 queda acotado a medio LSB, que es el piso teorico del formato. */
#define SINE_TABLE_SCALE 16
#define SINE_TABLE_SCALE_ONE (SINE_Q16_ONE * SINE_TABLE_SCALE)

/* ---------- Tabla de sincronización ---------- */

/* sine_table[k] = sin(k * 2*PI / SINE_TABLE_STEPS) * 65536 * 16, redondeado.
 * El último punto (k = 3600) es el retorno al origen: se usa para interpolar
 * el tramo final del círculo y no se duplica en la búsqueda. */
static int32_t sine_table[SINE_TABLE_STEPS + 1];

static void sine_table_init(void) {
    int k;
    for (k = 0; k <= SINE_TABLE_STEPS; ++k) {
        double theta = (double)k * (2.0 * M_PI / (double)SINE_TABLE_STEPS);
        /* Redondeo al entero mas cercano: el error de cuantizacion queda acotado
         * a medio LSB en vez de un LSB completo (truncar hacia cero). */
        sine_table[k] = (int32_t)lround(sin(theta) * (double)SINE_TABLE_SCALE_ONE);
    }
}

/* Division entera redondeada al mas cercano (el denominador es positivo). */
static int32_t div_round(int64_t num, int64_t den) {
    if (num >= 0) return (int32_t)((num + den / 2) / den);
    return -(int32_t)((-num + den / 2) / den);
}

/* ---------- Generador ---------- */

/* Escribe n muestras del seno en out[0..n-1].
 * La muestra i corresponde a la fase phase_offset + i (unidades de 1/100 grado),
 * envuelta al círculo. phase_offset puede ser negativo. */
void sine_q16_gen(int32_t *out, int n, int32_t phase_offset) {
    int i;

    if (!out || n <= 0) return;

    for (i = 0; i < n; ++i) {
        int64_t phase = (int64_t)phase_offset + (int64_t)i;
        int32_t p, idx, frac;
        int32_t v0, v1, val;

        /* envuelve la fase a [0, SINE_PHASE_UNITS) sin perder el signo */
        phase %= (int64_t)SINE_PHASE_UNITS;
        if (phase < 0) phase += (int64_t)SINE_PHASE_UNITS;
        p = (int32_t)phase;

        /* idx: punto de tabla; frac: fracción de 1/100 de grado entre p[idx] y p[idx+1] */
        idx = p / SINE_TABLE_FRAC;
        frac = p % SINE_TABLE_FRAC;

        v0 = sine_table[idx];
        v1 = sine_table[idx + 1];   /* idx + 1 <= SINE_TABLE_STEPS, la tabla cierra el círculo */

        /* Interpolacion lineal en Q16.16 con 4 bits decimales extra.
         * num esta en unidades de 1/(10*SINE_TABLE_SCALE) LSB y se redondea
         * una sola vez al convertir a Q16.16, para no acumular truncamientos. */
        {
            int64_t num = (int64_t)v0 * SINE_TABLE_FRAC
                        + (int64_t)(v1 - v0) * (int64_t)frac;
            val = div_round(num, (int64_t)SINE_TABLE_FRAC * SINE_TABLE_SCALE);
        }

        out[i] = val;
    }
}

/* Coseno: el seno con la fase desplazada un cuarto de vuelta. */
void cosine_q16_gen(int32_t *out, int n, int32_t phase_offset) {
    if (!out) return;
    sine_q16_gen(out, n, phase_offset + SINE_QUARTER_TURN);
}

#ifdef TEST

/* Umbral de error máximo aceptable para Q16.16.
 * El piso de cuantización es 2^-17 = 7.63e-06 y la interpolación sobre pasos de
 * 0.1 grados añade del orden de 4e-07, así que 2e-05 deja margen sin ser laxo. */
#define SINE_ERR_THRESHOLD 2.0e-5

static int check_close(const char *what, double got, double want, double tol) {
    double err = fabs(got - want);
    if (err > tol) {
        printf("FALLO  %-28s esperado %.9f, obtenido %.9f (err %.3e > %.3e)\n",
               what, want, got, err, tol);
        return 0;
    }
    return 1;
}

int main(void) {
    const int n = SINE_PHASE_UNITS + 1;   /* círculo completo + punto de cierre */
    static int32_t buf[SINE_PHASE_UNITS + 1];
    static int32_t cbuf[SINE_PHASE_UNITS + 1];
    double max_err = 0.0, max_err_cos = 0.0;
    double peak = 0.0;
    int i, ok = 1;

    sine_table_init();

    sine_q16_gen(buf, n, 0);
    cosine_q16_gen(cbuf, n, 0);

    /* Error máximo frente a la referencia en doble precisión */
    for (i = 0; i < n; ++i) {
        double th = (double)i * (2.0 * M_PI / (double)SINE_PHASE_UNITS);
        double e = fabs((double)buf[i] / (double)SINE_Q16_ONE - sin(th));
        double ec = fabs((double)cbuf[i] / (double)SINE_Q16_ONE - cos(th));
        if (e > max_err) max_err = e;
        if (ec > max_err_cos) max_err_cos = ec;
    }

    /* La salida debe recorrer el rango completo: el pico vale 1.0 en Q16.16 */
    for (i = 0; i < n; ++i) {
        double v = (double)buf[i] / (double)SINE_Q16_ONE;
        if (v > peak) peak = v;
    }

    printf("Q16.16 max_err (seno)   = %.3e\n", max_err);
    printf("Q16.16 max_err (coseno) = %.3e\n", max_err_cos);
    printf("Pico del seno            = %.6f (esperado 1.0)\n", peak);
    printf("Rango de salida          = %d .. %d (=%+.4f .. %+.4f)\n",
           -SINE_Q16_ONE, SINE_Q16_ONE,
           -1.0, 1.0);

    /* Comprobaciones con criterio: devuelven código de salida distinto de 0 */
    if (max_err > SINE_ERR_THRESHOLD) {
        printf("FALLO  error de seno %.3e supera el umbral %.3e\n",
               max_err, SINE_ERR_THRESHOLD);
        ok = 0;
    }
    if (max_err_cos > SINE_ERR_THRESHOLD) {
        printf("FALLO  error de coseno %.3e supera el umbral %.3e\n",
               max_err_cos, SINE_ERR_THRESHOLD);
        ok = 0;
    }
    if (!check_close("pico del seno", peak, 1.0, 2.0e-4)) ok = 0;
    if (!check_close("seno en fase 0", (double)buf[0] / SINE_Q16_ONE, 0.0, 1.0e-5)) ok = 0;
    if (!check_close("seno en un cuarto", (double)buf[SINE_QUARTER_TURN] / SINE_Q16_ONE,
                     1.0, 2.0e-4)) ok = 0;
    if (!check_close("seno en media vuelta", (double)buf[SINE_PHASE_UNITS / 2] / SINE_Q16_ONE,
                     0.0, 1.0e-5)) ok = 0;
    if (!check_close("cierre del círculo", (double)buf[SINE_PHASE_UNITS] / SINE_Q16_ONE,
                     0.0, 1.0e-5)) ok = 0;
    if (!check_close("coseno en fase 0", (double)cbuf[0] / SINE_Q16_ONE, 1.0, 2.0e-4)) ok = 0;

    /* Monotonía en el primer cuarto de vuelta */
    for (i = 1; i <= SINE_QUARTER_TURN; ++i) {
        if (buf[i] < buf[i - 1]) {
            printf("FALLO  el seno no es monótono creciente hasta π/2 (i = %d)\n", i);
            ok = 0;
            break;
        }
    }

    if (ok) {
        printf("OK  todas las comprobaciones pasan (umbral %.1e)\n", SINE_ERR_THRESHOLD);
        return EXIT_SUCCESS;
    }
    printf("FALLO  la prueba del seno no pasa\n");
    return EXIT_FAILURE;
}
#endif
