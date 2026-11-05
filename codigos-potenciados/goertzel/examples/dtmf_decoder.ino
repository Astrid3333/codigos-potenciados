/*
 * DTMF decoder con Goertzel en float para Arduino.
 *
 * Lee muestras de audio (simulado aquí con tonos generados),
 * aplica Goertzel a las 8 frecuencias DTMF y detecta la tecla pulsada.
 *
 * Configuración:
 *   fs = 8000 Hz  (frecuencia de muestreo)
 *   N  = 205      (ventana de 25.6 ms)
 *
 * Frecuencias DTMF:
 *   Filas:    697, 770, 852, 941 Hz
 *   Columnas: 1209, 1336, 1477, 1633 Hz
 */

#include <goertzel_arduino.h>

#define FS    8000.0f   /* frecuencia de muestreo (Hz) */
#define N     205       /* muestras por ventana (25.6 ms @ 8kHz) */

float samples[N];
int sample_idx = 0;

/* Generador de tono sintético para pruebas (simula entrada de micrófono) */
float generate_dtmf_tone(char key, unsigned long t_ms) {
    /* Tabla de frecuencias por tecla */
    static const float row_freq[4] = {697.0f, 770.0f, 852.0f, 941.0f};
    static const float col_freq[4] = {1209.0f, 1336.0f, 1477.0f, 1633.0f};

    int r = -1, c = -1;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (DTMF_KEYS[i][j] == key) { r = i; c = j; }
        }
    }
    if (r < 0 || c < 0) return 0.0f;

    float t = (float)t_ms / 1000.0f;
    return 0.5f * sinf(2.0f * PI * row_freq[r] * t) +
           0.5f * sinf(2.0f * PI * col_freq[c] * t);
}

/* Detectar tecla en el buffer actual de muestras */
char detect_dtmf(void) {
    float row_w[4], col_w[4];
    float row_mag[4], col_mag[4];

    const float row_freq[4] = {DTMF_ROW1, DTMF_ROW2, DTMF_ROW3, DTMF_ROW4};
    const float col_freq[4] = {DTMF_COL1, DTMF_COL2, DTMF_COL3, DTMF_COL4};

    for (int i = 0; i < 4; i++) {
        row_w[i] = goertzel_omega(row_freq[i], FS, N);
        col_w[i] = goertzel_omega(col_freq[i], FS, N);
    }

    for (int i = 0; i < 4; i++) {
        goertzel_cpx_t rc = goertzel_hybrid(samples, N, row_w[i]);
        goertzel_cpx_t cc = goertzel_hybrid(samples, N, col_w[i]);
        row_mag[i] = goertzel_mag(&rc);
        col_mag[i] = goertzel_mag(&cc);
    }

    /* Encontrar índices de pico */
    int row_idx = 0, col_idx = 0;
    for (int i = 1; i < 4; i++) {
        if (row_mag[i] > row_mag[row_idx]) row_idx = i;
        if (col_mag[i] > col_mag[col_idx]) col_idx = i;
    }

    return goertzel_detect_dtmf(row_mag, col_mag, row_idx, col_idx, 3.0f);
}

void setup() {
    Serial.begin(9600);
    while (!Serial) { /* esperar */ }
    Serial.println(F("DTMF decoder con Goertzel (float)"));
    Serial.println(F("Esperando tonos..."));
}

char last_key = 0;
unsigned long tone_start = 0;

void loop() {
    /* Simular recepción de una tecla DTMF cada 3 segundos */
    static const char *keys = "123456789*0#ABCD";
    static int key_idx = 0;
    static unsigned long last_tone = 0;

    unsigned long now = millis();

    if (now - last_tone > 3000) {
        char key = keys[key_idx];
        key_idx = (key_idx + 1) % 16;
        last_tone = now;
        tone_start = now;
        Serial.print(F("Enviando tecla: "));
        Serial.println(key);
    }

    /* Generar N muestras del tono actual */
    unsigned long t_elapsed = now - tone_start;
    if (t_elapsed < 1500) {  /* tono activo durante 1.5 s */
        char key = keys[(key_idx + 15) % 16];  /* tecla que se está enviando */
        for (int i = 0; i < N; i++) {
            unsigned long t_us = (unsigned long)((now - tone_start) * 1000UL) + (unsigned long)(i * 125UL);
            samples[i] = generate_dtmf_tone(key, t_us / 1000);
        }
        sample_idx = N;

        /* Detectar */
        char detected = detect_dtmf();
        if (detected && detected != last_key) {
            Serial.print(F("Tecla detectada: "));
            Serial.println(detected);
            last_key = detected;
        } else if (!detected) {
            last_key = 0;
        }
    }

    delay(20);
}
