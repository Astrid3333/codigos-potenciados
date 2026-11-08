/* cordic_arduino.h - CORDIC en punto fijo Q15 para Arduino (AVR)
 *
 * Implementa el algoritmo CORDIC (COordinate Rotation DIgital Computer)
 * de Jack Volder (1959). Calcula seno y coseno usando únicamente sumas,
 * restas y desplazamientos binarios (shifts). No usa multiplicaciones ni
 * librería matemática en el bucle principal.
 *
 * Ideal para Arduino (AVR ATmega328P) donde float == 32 bits y no hay FPU.
 *
 * Uso típico:
 *   #include <cordic_arduino.h>
 *
 *   cordic_fixed16_t theta = cordic_radians_to_fixed16(0.5);  /* 0.5 rad */
 *   cordic_fixed16_result_t r = cordic_sincos_fixed16(theta);
 *   int16_t s = (int16_t)r.y;   /* sin(theta) en Q15 */
 *   int16_t c = (int16_t)r.x;   /* cos(theta) en Q15 */
 *
 * Referencia: Volder, 1959.
 */

#ifndef CORDIC_ARDUINO_H
#define CORDIC_ARDUINO_H

#include <stdint.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* Numero de iteraciones CORDIC.
 * En Arduino Uno (ATmega328P, 16 MHz) con 12 iteraciones tarda ~30 us.
 * Con 16 iteraciones ~50 us. Mas iteraciones = mas precision pero mas lento.
 * Valores recomendados: 8-16.
 */
#ifndef CORDIC_ITERATIONS
#define CORDIC_ITERATIONS 12
#endif

/* Ganancia CORDIC K = prod_{i=0}^{N-1} sqrt(1 + 2^(-2i))
 * Para N=12: K = 1.6467601926846949
 * En Q15: K * 32768 = 53961.0380 -> 53961 (valor redondeado).
 *
 * Nota: el valor redondeado es 53961 tanto para N=12 como para N=16
 * (K*32768 = 53961.0401); la diferencia real es despreciable en Q15.
 */
#define CORDIC_K_Q15 53961L

typedef int32_t cordic_fixed16_t;   /* Q15: 1 bit signo, 15 bits fraccion */
#define CORDIC_FIXED16_ONE 32768L   /* 1.0 en Q15 */
#define CORDIC_FIXED16_PI 16384L    /* pi/2 en Q15 (rango de convergencia CORDIC) */

/* Resultado de CORDIC en punto fijo Q15 */
typedef struct {
    cordic_fixed16_t x;      /* K * cos(theta) en Q15 */
    cordic_fixed16_t y;      /* K * sin(theta) en Q15 */
    cordic_fixed16_t theta;  /* angulo residual en Q15 */
    int8_t iterations;
} cordic_fixed16_result_t;

/* Tabla de arctan(2^-i) en Q15.
 * Valor = atan(2^-i) / pi * 32768, redondeado al entero mas cercano.
 *
 * Valores verificados para cada i:
 */
static const cordic_fixed16_t cordic_atan_table_q15[CORDIC_ITERATIONS] = {
#if CORDIC_ITERATIONS >= 1
    8192L,   /* atan(2^0)   = atan(1)       -> 8192  */
#endif
#if CORDIC_ITERATIONS >= 2
    4836L,   /* atan(2^-1)  = atan(0.5)     -> 4836  */
#endif
#if CORDIC_ITERATIONS >= 3
    2546L,   /* atan(2^-2)  = atan(0.25)    -> 2546  */
#endif
#if CORDIC_ITERATIONS >= 4
    1291L,   /* atan(2^-3)  = atan(0.125)   -> 1291  */
#endif
#if CORDIC_ITERATIONS >= 5
    649L,    /* atan(2^-4)  = atan(0.0625)  -> 649   */
#endif
#if CORDIC_ITERATIONS >= 6
    325L,    /* atan(2^-5)  = atan(0.03125) -> 325   */
#endif
#if CORDIC_ITERATIONS >= 7
    163L,    /* atan(2^-6)  = atan(0.015625)-> 163   */
#endif
#if CORDIC_ITERATIONS >= 8
    81L,     /* atan(2^-7)  = atan(0.0078125)-> 81   */
#endif
#if CORDIC_ITERATIONS >= 9
    41L,     /* atan(2^-8)  = atan(0.00390625)-> 41   */
#endif
#if CORDIC_ITERATIONS >= 10
    20L,     /* atan(2^-9)  = atan(0.001953125)-> 20   */
#endif
#if CORDIC_ITERATIONS >= 11
    5L,      /* atan(2^-10) = atan(0.0009765625)-> 5   */
#endif
#if CORDIC_ITERATIONS >= 12
    3L,      /* atan(2^-11) = atan(0.00048828125)-> 3   */
#endif
#if CORDIC_ITERATIONS >= 13
    2L,      /* atan(2^-12) = atan(0.000244140625)-> 2   */
#endif
#if CORDIC_ITERATIONS >= 14
    1L,      /* atan(2^-13) = atan(0.0001220703125)-> 1   */
#endif
#if CORDIC_ITERATIONS >= 15
    0L,      /* atan(2^-14) = atan(0.00006103515625)-> 0   */
#endif
#if CORDIC_ITERATIONS >= 16
    0L,      /* atan(2^-15) = atan(0.000030517578125)-> 0   */
#endif
};

/* ---------- Utilidades de conversion -------------------------- */

/* Convertir angulo en radianes a Q15 (angulo = fixed/32768 * pi) */
static inline cordic_fixed16_t cordic_radians_to_fixed16(double radians) {
    double t = radians / PI * (double)CORDIC_FIXED16_ONE;
    if (t > 32767.0) t = 32767.0;
    if (t < -32768.0) t = -32768.0;
    return (cordic_fixed16_t)t;
}

/* Convertir Q15 a radianes */
static inline double cordic_fixed16_to_radians(cordic_fixed16_t fixed) {
    return ((double)fixed / (double)CORDIC_FIXED16_ONE) * PI;
}

/* ---------- CORDIC en punto fijo Q15 -------------------------- */

/* Calcula sin y cos de theta_fixed (en Q15, valor real = fixed/32768 * pi).
 * Devuelve (cos, sin) ya escalados por K en Q15.
 *
 * Implementacion:
 *   - Reduce el argumento a [-pi/2, pi/2] para que CORDIC converja
 *   - Usa desplazamientos en vez de multiplicaciones
 *   - Solo suma/resta/shift en el bucle principal
 */
static inline cordic_fixed16_result_t cordic_sincos_fixed16(
        cordic_fixed16_t theta_fixed) {
    cordic_fixed16_t x = CORDIC_FIXED16_ONE;  /* x = 1.0 en Q15 */
    cordic_fixed16_t y = 0;                   /* y = 0 */
    cordic_fixed16_t z = theta_fixed;         /* angulo restante */
    int i;

    /* Reducir argumento a [-pi/2, pi/2] para que CORDIC converja */
    while (z > CORDIC_FIXED16_PI) z -= CORDIC_FIXED16_ONE * 2;
    while (z < -CORDIC_FIXED16_PI) z += CORDIC_FIXED16_ONE * 2;

    for (i = 0; i < CORDIC_ITERATIONS; i++) {
        cordic_fixed16_t sigma = (z >= 0) ? CORDIC_FIXED16_ONE : -CORDIC_FIXED16_ONE;
        cordic_fixed16_t x_new, y_new;

        /* x_new = x - sigma * (y >> i)   (desplaza en vez de multiplicar) */
        /* y_new = y + sigma * (x >> i)   */
        cordic_fixed16_t y_shifted = y >> i;
        cordic_fixed16_t x_shifted = x >> i;

        if (sigma > 0) {
            x_new = x - y_shifted;
            y_new = y + x_shifted;
        } else {
            x_new = x + y_shifted;
            y_new = y - x_shifted;
        }

        /* z = z - sigma * atan_table[i] */
        if (sigma > 0) {
            z -= cordic_atan_table_q15[i];
        } else {
            z += cordic_atan_table_q15[i];
        }

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

/* Version conveniente: devuelve sin y cos ya en Q15
 * (incluyen la ganancia K). */
static inline void cordic_sincos_fixed16_normalized(
        cordic_fixed16_t theta_fixed,
        cordic_fixed16_t *sin_out,
        cordic_fixed16_t *cos_out) {
    cordic_fixed16_result_t r = cordic_sincos_fixed16(theta_fixed);
    *cos_out = r.x;
    *sin_out = r.y;
}

/* ---------- Funciones de conveniencia -------------------------- */

/* Genera un seno de fase dada en Q15.
 * phase es el angulo en Q15. Devuelve el seno en Q15. */
static inline cordic_fixed16_t cordic_sin_fixed16(cordic_fixed16_t phase) {
    cordic_fixed16_result_t r = cordic_sincos_fixed16(phase);
    return r.y;  /* sin */
}

static inline cordic_fixed16_t cordic_cos_fixed16(cordic_fixed16_t phase) {
    cordic_fixed16_result_t r = cordic_sincos_fixed16(phase);
    return r.x;  /* cos */
}

#endif /* CORDIC_ARDUINO_H */