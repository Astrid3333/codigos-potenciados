/* cordic.c - Implementación del algoritmo CORDIC (1959)
 *
 * Implementa rotación de vector (1,0) por un ángulo theta usando
 * únicamente sumas, restas y desplazamientos (shifts). No usa
 * multiplicaciones ni funciones trigonométricas en el bucle principal.
 *
 * Referencia: Volder, 1959.
 */

#include "cordic.h"
#include <math.h>   /* ldexp */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- Tabla de arctan(2^-i) en radianes ---------------- */

const double cordic_atan_table[CORDIC_ITERATIONS] = {
    0.7853981633974483,   /* atan(2^0)   = atan(1)       = pi/4       */
    0.4636476090008061,   /* atan(2^-1)  = atan(0.5)     */
    0.24497866312686414,  /* atan(2^-2)  = atan(0.25)    */
    0.12435499454676144,  /* atan(2^-3)  = atan(0.125)   */
    0.06241880999595735,  /* atan(2^-4)  = atan(0.0625)  */
    0.031239833430268277, /* atan(2^-5)  = atan(0.03125) */
    0.015623728620476831, /* atan(2^-6)  = atan(0.015625)*/
    0.007812341060103611, /* atan(2^-7)  = atan(0.0078125)*/
    0.003906230131966972, /* atan(2^-8)  = atan(0.00390625)*/
    0.001953122517477657, /* atan(2^-9)  = atan(0.001953125)*/
    0.000976562187454314, /* atan(2^-10) = atan(0.0009765625)*/
    0.000488281211184362, /* atan(2^-11) = ...           */
    0.000244140620149136, /* atan(2^-12)           */
    0.000122070312475822, /* atan(2^-13)           */
    0.000061035156157323, /* atan(2^-14)           */
    0.000030517578114532, /* atan(2^-15)           */
};

/* ---------- Implementación pública ----------------------------- */

cordic_result_t cordic_sincos(double theta) {
    double x = 1.0, y = 0.0, z = theta;
    int i;

    /* Normalizar a [-pi, pi] */
    while (theta > M_PI) theta -= 2.0 * M_PI;
    while (theta < -M_PI) theta += 2.0 * M_PI;

    /* Reducir a [-pi/2, pi/2] usando identidades trigonometricas */
    int sign_cos = 1, sign_sin = 1;
    if (theta > M_PI / 2.0) {
        theta = M_PI - theta;
        sign_cos = -1;
    } else if (theta < -M_PI / 2.0) {
        theta = -M_PI - theta;
        sign_cos = -1;
    }
    z = theta;

    for (i = 0; i < CORDIC_ITERATIONS; i++) {
        double sigma = (z >= 0.0) ? 1.0 : -1.0;
        double factor = ldexp(1.0, -i);
        double x_next = x - sigma * y * factor;
        double y_next = y + sigma * x * factor;
        z = z - sigma * cordic_atan_table[i];
        x = x_next;
        y = y_next;
    }

    return (cordic_result_t){
        x * sign_cos,
        y * sign_sin,
        z,
        CORDIC_ITERATIONS
    };
}

/* ---------- Implementación en punto fijo Q15 ---------------- */

cordic_fixed16_result_t cordic_sincos_fixed16(cordic_fixed16_t theta_fixed) {
    /* theta_fixed está en Q15: valor real = theta_fixed / 32768 * pi */
    double theta = ((double)theta_fixed / (double)CORDIC_FIXED16_ONE) * M_PI;

    /* Usar la implementación en punto flotante interna y convertir */
    cordic_result_t r = cordic_sincos(theta);
    cordic_fixed16_result_t result;

    result.x = (cordic_fixed16_t)(r.x * (double)CORDIC_FIXED16_ONE);
    result.y = (cordic_fixed16_t)(r.y * (double)CORDIC_FIXED16_ONE);
    result.theta = (cordic_fixed16_t)((r.theta / M_PI) * (double)CORDIC_FIXED16_ONE);
    result.iterations = r.iterations;

    return result;
}

/* ---------- Utilidad: ángulo en radianes a Q15 ---------------- */

cordic_fixed16_t cordic_radians_to_fixed16(double radians) {
    double t = radians / M_PI * (double)CORDIC_FIXED16_ONE;
    if (t > 32767.0) t = 32767.0;
    if (t < -32768.0) t = -32768.0;
    return (cordic_fixed16_t)t;
}

double cordic_fixed16_to_radians(cordic_fixed16_t fixed) {
    return ((double)fixed / (double)CORDIC_FIXED16_ONE) * M_PI;
}