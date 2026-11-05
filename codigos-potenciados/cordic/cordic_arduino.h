/* cordic_arduino.h - CORDIC en punto fijo Q15 para Arduino (AVR)
 *
 * Implementación del algoritmo CORDIC optimizada para Arduino (AVR ATmega328P)
 * donde no hay FPU y float == 32 bits (lento). Usa punto fijo Q15 para
 * máxima velocidad.
 *
 * Uso típico:
 *   #include <cordic_arduino.h>
 *
 *   // Generar seno de 440 Hz (la nota A4) en tiempo real:
 *   cordic_fixed16_t theta = cordic_degrees_to_fixed16(45.0);  // 45 grados
 *   cordic_fixed16_result_t r = cordic_sincos_fixed16(theta);
 *   int16_t seno = (int16_t)(r.y);  /* valor en Q15, rango [-32768, 32767] */
 *
 * Todas las operaciones usan sumas, restas y desplazamientos (shifts).
 * No hay multiplicaciones ni librería matemática en el bucle principal.
 *
 * Referencia: Volder, 1959.
 */

#ifndef CORDIC_ARDUINO_H
#define CORDIC_ARDUINO_H

#include <stdint.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* Número de iteraciones CORDIC.
 * En Arduino Uno (ATmega328P, 16 MHz) con 12 iteraciones tarda ~30 µs.
 * Con 16 iteraciones ~50 µs. Más iteraciones = más precisión pero más lento.
 */
#ifndef CORDIC_ITERATIONS
#define CORDIC_ITERATIONS 12
#endif

/* Ganancia CORDIC K para N iteraciones.
 * Para N=12: K ≈ 1.6467602581210654
 * En Q15: K * 32768 ≈ 54033 */
#define CORDIC_K_Q15 54033L

typedef int32_t cordic_fixed16_t;   /* Q15: 1 bit signo, 15 bits fracción */
#define CORDIC_FIXED16_ONE 32768L   /* 1.0 en Q15 */

typedef struct {
    cordic_fixed16_t x;      /* K * cos(theta) en Q15 */
    cordic_fixed16_t y;      /* K * sin(theta) en Q15 */
    cordic_fixed16_t theta;  /* ángulo residual en Q15 */
    int8_t iterations;
} cordic_fixed16_result_t;

/* Tabla de arctan(2^-i) en Q15 (valor entero que representa
 * atan(2^-i) / pi * 32768) */
static const cordic_fixed16_t cordic_atan_table_q15[CORDIC_ITERATIONS] = {
#if CORDIC_ITERATIONS >= 1
    8192L,   /* atan(1)   = pi/4   -> 8192  */
#endif
#if CORDIC_ITERATIONS >= 2
    4018L,   /* atan(0.5)                         */
#endif
#if CORDIC_ITERATIONS >= 3
    2128L,   /* atan(0.25)                        */
#endif
#if CORDIC_ITERATIONS >= 4
    1074L,   /* atan(0.125)                       */
#endif
#if CORDIC_ITERATIONS >= 5
    540L,    /* atan(0.0625)                      */
#endif
#if CORDIC_ITERATIONS >= 6
    271L,    /* atan(0.03125)                     */
#endif
#if CORDIC_ITERATIONS >= 7
    136L,    /* atan(0.015625)                    */
#endif
#if CORDIC_ITERATIONS >= 8
    68L,     /* atan(0.0078125)                   */
#endif
#if CORDIC_ITERATIONS >= 9
    34L,     /* atan(0.00390625)                  */
#endif
#if CORDIC_ITERATIONS >= 10
    17L,     /* atan(0.001953125)                 */
#endif
#if CORDIC_ITERATIONS >= 11
    8L,      /* atan(0.0009765625)                */
#endif
#if CORDIC_ITERATIONS >= 12
    4L,      /* atan(0.00048828125)               */
#endif
#if CORDIC_ITERATIONS >= 13
    2L,      /* atan(0.000244140625)              */
#endif
#if CORDIC_ITERATIONS >= 14
    1L,      /* atan(0.0001220703125)             */
#endif
#if CORDIC_ITERATIONS >= 15
    0L,      /* atan(0.00006103515625) -> 0 en Q15*/
#endif
#if CORDIC_ITERATIONS >= 16
    0L,      /* atan(0.000030517578125) -> 0      */
#endif
};

/* Convertir grados a Q15 (ángulo) */
static inline cordic_fixed16_t cordic_degrees_to_fixed16(float degrees) {
    double rad = degrees * PI / 180.0;
    return (cordic_fixed16_t)((rad / PI) * (double)CORDIC_FIXED16_ONE);
}

/* Convertir radianes a Q15 */
static inline cordic_fixed16_t cordic_radians_to_fixed16(double radians) {
    return (cordic_fixed16_t)((radians / PI) * (double)CORDIC_FIXED16_ONE);
}

/* Convertir Q15 a grados */
static inline float cordic_fixed16_to_degrees(cordic_fixed16_t fixed) {
    return (float)fixed / (float)CORDIC_FIXED16_ONE * 180.0f;
}

/* Convertir Q15 a radianes */
static inline double cordic_fixed16_to_radians(cordic_fixed16_t fixed) {
    return ((double)fixed / (double)CORDIC_FIXED16_ONE) * PI;
}

/* ---------- CORDIC en punto fijo Q15 -------------------------- */

/* Calcula sin y cos de theta_fixed (en Q15, radianes = fixed/32768 * pi).
 * Devuelve (cos, sin) ya escalados por K en Q15. */
static inline cordic_fixed16_result_t cordic_sincos_fixed16(
        cordic_fixed16_t theta_fixed) {
    cordic_fixed16_t x = CORDIC_FIXED16_ONE;  /* x = 1.0 en Q15 */
    cordic_fixed16_t y = 0;                   /* y = 0 */
    cordic_fixed16_t z = theta_fixed;         /* ángulo restante */
    int i;

    for (i = 0; i < CORDIC_ITERATIONS; i++) {
        cordic_fixed16_t sigma = (z >= 0) ? CORDIC_FIXED16_ONE : -CORDIC_FIXED16_ONE;
        cordic_fixed16_t x_new, y_new;
        cordic_fixed16_t shift_val;

        /* shift_val = sigma * 2^{-i} en Q15 */
        shift_val = sigma >> i;

        /* x_new = x - shift_val * y */
        x_new = x - ((shift_val * y) >> 15);
        /* y_new = y + shift_val * x_old  (usar x ANTES de actualizar) */
        y_new = y + ((shift_val * x) >> 15);

        /* z = z - sigma * atan_table[i] */
        z = z - ((sigma * cordic_atan_table_q15[i]) >> 15);

        x = x_new;
        y = y_new;
    }

    return (cordic_fixed16_result_t){
        .x = x,
        .y = y,
        .theta = z,
        .iterations = CORDIC_ITERATIONS
    };
}

/* Versión conveniente: devuelve sin y cos ya en Q15
 * (incluyen la ganancia K). */
static inline void cordic_sincos_fixed16_normalized(
        cordic_fixed16_t theta_fixed,
        cordic_fixed16_t *sin_out,
        cordic_fixed16_t *cos_out) {
    cordic_fixed16_result_t r = cordic_sincos_fixed16(theta_fixed);
    *cos_out = r.x;
    *sin_out = r.y;
}

/* ---------- Ejemplo: generar tono senoidal --------------------- */

/* Genera un seno de frecuencia dada en Q15.
 * phase: fase actual en Q15 (se incrementa cada muestra).
 * Devuelve el valor del seno en Q15. */
static inline cordic_fixed16_t cordic_sin_fixed16(cordic_fixed16_t phase) {
    cordic_fixed16_result_t r = cordic_sincos_fixed16(phase);
    return r.y;  /* sin */
}

static inline cordic_fixed16_t cordic_cos_fixed16(cordic_fixed16_t phase) {
    cordic_fixed16_result_t r = cordic_sincos_fixed16(phase);
    return r.x;  /* cos */
}

#endif /* CORDIC_ARDUINO_H */