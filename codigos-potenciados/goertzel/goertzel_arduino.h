#ifndef GOERTZEL_ARDUINO_H
#define GOERTZEL_ARDUINO_H

#include <Arduino.h>
#include <math.h>

/* Goertzel en float para Arduino (AVR: float == 32 bits, double == float)
 *
 * Tres formulaciones:
 *   - goertzel_k()   : forma original K = 2*cos(w), zona media de w
 *   - goertzel_alpha(): forma para w pequeño (cerca de 0)
 *   - goertzel_hybrid(): alpha si w < pi/2, beta si w >= pi/2 (recomendada)
 *
 * Uso típico para DTMF (fs = 8000 Hz, N = 205):
 *   float mag = goertzel_mag(goertzel_hybrid(samples, N, w));
 *   donde w = 2*PI*freq/fs.
 */

typedef struct {
    float re;
    float im;
} goertzel_cpx_t;

static inline float goertzel_mag(const goertzel_cpx_t *c) {
    return sqrtf(c->re * c->re + c->im * c->im);
}

/* Goertzel K: forma original. Mejor para pi/4 <= w <= 3pi/4. */
static inline goertzel_cpx_t goertzel_k(const float *x, int n, float w) {
    float K = 2.0f * cosf(w);
    float s1 = 0.0f, s2 = 0.0f, s;
    for (int i = 0; i < n; ++i) {
        s = x[i] + K * s1 - s2;
        s2 = s1;
        s1 = s;
    }
    s = K * s1 - s2;  /* x[N] = 0 */
    goertzel_cpx_t c;
    c.re = s - cosf(w) * s1;
    c.im = sinf(w) * s1;
    return c;
}

/* Goertzel alpha: mejor para w pequeno (cerca de 0). */
static inline goertzel_cpx_t goertzel_alpha(const float *x, int n, float w) {
    float al = 4.0f * sinf(w / 2.0f) * sinf(w / 2.0f);
    float s = 0.0f, d = 0.0f;
    for (int i = 0; i < n; ++i) {
        d = d + x[i] - al * s;
        s = s + d;
    }
    d = d - al * s;
    s = s + d;
    float s1 = s - d;
    goertzel_cpx_t c;
    c.re = s - cosf(w) * s1;
    c.im = sinf(w) * s1;
    return c;
}

/* Goertzel híbrida: alpha si w < pi/2, beta si w >= pi/2. Recomendada general. */
static inline goertzel_cpx_t goertzel_hybrid(const float *x, int n, float w) {
    if (w < PI / 2.0f)
        return goertzel_alpha(x, n, w);
    /* beta: 4*cos^2(w/2) */
    float be = 4.0f * cosf(w / 2.0f) * cosf(w / 2.0f);
    float s = 0.0f, d = 0.0f;
    for (int i = 0; i < n; ++i) {
        d = x[i] + be * s - d;
        s = d - s;
    }
    d = be * s - d;
    s = d - s;
    float s1 = d - s;
    goertzel_cpx_t c;
    c.re = s - cosf(w) * s1;
    c.im = sinf(w) * s1;
    return c;
}

/* Frecuencias DTMF estándar (Hz) */
#define DTMF_ROW1 697.0f
#define DTMF_ROW2 770.0f
#define DTMF_ROW3 852.0f
#define DTMF_ROW4 941.0f
#define DTMF_COL1 1209.0f
#define DTMF_COL2 1336.0f
#define DTMF_COL3 1477.0f
#define DTMF_COL4 1633.0f

/* Matriz DTMF: filas x columnas */
static const char DTMF_KEYS[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* Calcular omega para una frecuencia dada fs y N */
static inline float goertzel_omega(float freq, float fs, int n) {
    return 2.0f * PI * freq / fs;
}

/* Detectar tecla DTMF a partir de magnitudes de filas y columnas.
 * Devuelve el caracter detectado o 0 si no hay tecla clara.
 * row_mag[4] y col_mag[4] son las magnitudes de Goertzel para cada frecuencia.
 * row_idx y col_idx son los índices de fila/columna con mayor magnitud.
 * threshold: ratio mínimo entre el pico y el segundo pico para validar. */
static inline char goertzel_detect_dtmf(const float *row_mag, const float *col_mag,
                                         int row_idx, int col_idx,
                                         float threshold) {
    if (row_idx < 0 || row_idx > 3 || col_idx < 0 || col_idx > 3)
        return 0;

    float row_max = row_mag[row_idx];
    float col_max = col_mag[col_idx];

    /* Encontrar segundo pico en filas y columnas */
    float row2 = 0.0f, col2 = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (i != row_idx && row_mag[i] > row2) row2 = row_mag[i];
        if (i != col_idx && col_mag[i] > col2) col2 = col_mag[i];
    }

    /* Validar que haya un pico claro en fila y columna */
    if (row2 > 0.0f && row_max / row2 < threshold) return 0;
    if (col2 > 0.0f && col_max / col2 < threshold) return 0;

    /* Validar que ambas magnitudes sean significativas */
    float total = 0.0f;
    for (int i = 0; i < 4; i++) {
        total += row_mag[i] + col_mag[i];
    }
    if (total <= 0.0f) return 0;
    if ((row_max + col_max) / total < 0.3f) return 0;

    return DTMF_KEYS[row_idx][col_idx];
}

#endif /* GOERTZEL_ARDUINO_H */