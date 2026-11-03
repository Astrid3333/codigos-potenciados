/*)
(* sine_fixed.c -- Seno Q16.16 con tabla incremental
 * Usa fase incremental para evitar overflow de int
 * Compilar: cc -std=c99 -O2 -DTEST sine_fixed.c -o sine_fixed -lm
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Constants */

#define SINE_STEPS 36000
#define Q16_ONE 65536
#define H_INC 411775  // fase incrementos por paso (411775/36000 ≈ 11.44 grados)

/* Tabla de sincronización - 3600 puntos (10x interp sobre 36000) */
static int32_t sine_table[3600 + 1];

static void sine_table_init(void) {
    int i;
    for (i = 0; i <= 3600; ++i) {
        double theta = i * 10.0 * (2.0 * M_PI / 36000.0);
        sine_table[i] = (int32_t)(sin(theta) * 65536.0);
    }
}

/* Generador de seno Q16.16 usando tabla con fase incremental */
static void sine_q16_gen(int16_t *out, int n) {
    int phase = 0;  // fase en unidades de 1/10 grado * 3600/36000
    int idx = 0;     // índice de tabla actual
    int frac = 0;    // fracción de interpolación 0-9
    int i;
    
    if (n <= 0) return;
    out[0] = 0;
    if (n == 1) return;
    
    for (i = 1; i < n; ++i) {
        // Avanzar fase: añadir 11 pasos de tabla + ajustar
        // 411775/36000 ≈ 11.44, así que avanzamos 11 y corregimos
        phase += 11438;  // redondeado: 411775*10/36000
        // Normalizar fase a rango 0-35999
        while (phase >= 36000) phase -= 36000;
        
        // Actualizar índice de tabla y fracción
        idx = phase % 3600;  // 3600 puntos en tabla
        frac = (phase / 3600) % 10;  // fracción 0-9
        
        // Interpolación lineal entre tabla[idx] y tabla[idx+1]
        int32_t v0 = sine_table[idx];
        int32_t v1 = sine_table[(idx + 1) % 3600];
        int32_t val = v0 + ((v1 - v0) * frac) / 10;
        
        // Recortar a int16_t Q16.16
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        
        out[i] = (int16_t)val;
    }
}

#ifdef TEST

int main(void) {
    int16_t buf[36001];
    double max_err16 = 0.0;
    int i;
    sine_table_init();
    sine_q16_gen(buf, 36001);
    for (i = 0; i <= 36000; ++i) {
        double th = i * 2.0 * M_PI / 36000.0;
        double val = buf[i] / 65536.0;
        double ref = sin(th);
        double err = fabs(val - ref);
        if (err > max_err16) max_err16 = err;
    }
    printf("Q16.16 max_err (tabla incremental): %.2e\n", max_err16);
    printf("Rango output: %d a %d (=%.*f a %.*f)\
", -32768, 32767, 0, -32768.0/65536.0, 0, 32767.0/65536.0);
    return 0;
}
#endif
