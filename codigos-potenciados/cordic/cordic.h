/* cordic.h - Algoritmo CORDIC (1959): sin/cos con sumas y desplazamientos
 *
 * Implementa el algoritmo CORDIC (COordinate Rotation DIgital Computer)
 * de Jack Volder (1959). Rotaciona un vector (1,0) por un ángulo θ usando
 * únicamente sumas, restas y desplazamientos binarios (shifts). No requiere
 * multiplicaciones ni tabla de senos/cosenos: solo una tabla de
 * arctan(2^-i) y la ganancia K.
 *
 * Modos de operación:
 *   - ROTACIÓN: partir de (1,0) y rotar por theta → (cos θ, sin θ)
 *   - VECTORIAL: partir de (x,y) y rotar hasta y=0 → (magnitud, ángulo)
 *
 * Implementaciones:
 *   - cordic_sincos_float(): punto flotante (C estándar)
 *   - cordic_sincos_fixed16(): punto fijo Q15 (Arduino/AVR sin FPU)
 *
 * Coeficientes:
 *   - K = producto de sqrt(1 + 2^(-2i)) para i = 0..N-1  ≈ 1.646760258121
 *   - atan_table[i] = atan(2^-i) en radianes
 *
 * Recomendación: con 16 iteraciones se alcanza ~1e-5 en float.
 * Para Arduino se recomiendan 12-16 iteraciones por velocidad/memoria.
 */

#ifndef CORDIC_H
#define CORDIC_H

#include <stddef.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Número de iteraciones CORDIC. Más iteraciones = más precisión.
 * Con 16 iteraciones se alcanza ~1e-5 en float.
 * Para Arduino se recomiendan 12-16 por velocidad/memoria. */
#define CORDIC_ITERATIONS 16

/* Ganancia CORDIC: K = prod_{i=0}^{N-1} sqrt(1 + 2^(-2i))
 * Para N=16: K ≈ 1.6467602581210654
 * Se usa para normalizar: x_final = K * cos(theta),
 * y_final = K * sin(theta). Dividiendo por K se obtiene el valor exacto. */
#define CORDIC_K 1.6467602581210654

/* Estructura de resultado: coordenadas rotadas y ángulo residual */
typedef struct {
    double x;      /* coeficiente escalado por K: K * cos(theta) */
    double y;      /* coeficiente escalado por K: K * sin(theta) */
    double theta;  /* ángulo residual restante */
    int iterations;
} cordic_result_t;

/* Punto fijo Q15: 1 bit signo, 15 bits fracción.
 * Valor real = valor_fijo / 32768.0 */
typedef int32_t cordic_fixed16_t;

#define CORDIC_FIXED16_ONE 32768  /* 1.0 en Q15 */
#define CORDIC_FIXED16_K   53961  /* K * 32768 ≈ 53961 (N=12 y N=16) */

/* Resultado en punto fijo Q15 */
typedef struct {
    cordic_fixed16_t x;     /* K * cos(theta) en Q15 */
    cordic_fixed16_t y;     /* K * sin(theta) en Q15 */
    cordic_fixed16_t theta; /* ángulo residual en Q15 */
    int iterations;
} cordic_fixed16_result_t;

/* ---------- Funciones públicas (punto flotante) ---------------- */

/* Calcula sin y cos de theta usando CORDIC en rotación.
 * theta en radianes. Devuelve (cos, sin) escalados por K;
 * dividir por CORDIC_K para obtener el valor exacto. */
cordic_result_t cordic_sincos(double theta);

/* Versión conveniente: devuelve solo sin y cos ya normalizados
 * (dividido por K). */
static inline void cordic_sincos_normalized(double theta,
                                            double *sin_out,
                                            double *cos_out) {
    cordic_result_t r = cordic_sincos(theta);
    *cos_out = r.x / CORDIC_K;
    *sin_out = r.y / CORDIC_K;
}

/* ---------- Funciones públicas (punto fijo Q15) ------------------ */

/* CORDIC en punto fijo Q15. theta en Q15 (valor entero que representa
 * radianes: valor_real = theta_fixed / 32768 * pi). */
cordic_fixed16_result_t cordic_sincos_fixed16(cordic_fixed16_t theta_fixed);

/* Versión conveniente: devuelve sin y cos ya normalizados por K
 * (devueltos en Q15). */
static inline void cordic_sincos_fixed16_normalized(
        cordic_fixed16_t theta_fixed,
        cordic_fixed16_t *sin_out,
        cordic_fixed16_t *cos_out) {
    cordic_fixed16_result_t r = cordic_sincos_fixed16(theta_fixed);
    *cos_out = r.x;  /* ya está en Q15 con K incluido */
    *sin_out = r.y;  /* ya está en Q15 con K incluido */
}

/* ---------- Utilidad: conversión de ángulo ---------------- */

/* Convierte radianes a Q15 (theta_fixed = radians / pi * 32768),
 * saturando al rango representable [-32768, 32767]. */
cordic_fixed16_t cordic_radians_to_fixed16(double radians);

/* Convierte de vuelta a radianes: radians = theta_fixed / 32768 * pi. */
double cordic_fixed16_to_radians(cordic_fixed16_t fixed);

/* ---------- Tabla de arctan (para referencia / depuración) ---------- */

/* atan_table[i] = atan(2^-i) en radianes, i = 0..CORDIC_ITERATIONS-1 */
extern const double cordic_atan_table[CORDIC_ITERATIONS];

#endif /* CORDIC_H */