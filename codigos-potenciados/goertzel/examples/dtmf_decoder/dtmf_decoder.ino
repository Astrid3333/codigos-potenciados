/*
 * DTMF decoder con Goertzel en float para Arduino.
 *
 * Lee muestras de audio (simulado o de micrófono), aplica Goertzel a las
 * 8 frecuencias DTMF y detecta la tecla pulsada.
 *
 * Configuración:
 *   fs = 8000 Hz  (frecuencia de muestreo)
 *   N  = 205      (ventana de 25.6 ms)
 *
 * Frecuencias DTMF:
 *   Filas:    697, 770, 852, 941 Hz
 *   Columnas: 1209, 1336, 1477, 1633 Hz
 *
 * Modo de uso:
 *   - SIMULATED (por defecto): genera tonos sintéticos para probar.
 *   - MICROPHONE: lee de analogRead(A0) a 8 kHz usando temporizador.
 *
 * Para usar con micrófono real:
 *   #define USE_MICROPHONE 1
 *   Conecta el micrófono a A0 a través de un condensador y divisor
 *   de tensión para centrar la señal en ~2.5 V (medio rango del ADC).
 */

#include <goertzel_arduino.h>

#define FS        8000.0f   /* frecuencia de muestreo (Hz) */
#define N         205       /* muestras por ventana (25.6 ms @ 8kHz) */
#define USE_MICROPHONE 0    /* 1 = leer de analogRead, 0 = tono simulado */

float samples[N];
int sample_idx = 0;

/* Tabla de frecuencias DTMF */
static const float ROW_FREQ[4] = {697.0f, 770.0f, 852.0f, 941.0f};
static const float COL_FREQ[4] = {1209.0f, 1336.0f, 1477.0f, 1633.0f};

/* Generador de tono sintético para pruebas (simula entrada de micrófono) */
float generate_dtmf_tone(char key, float t_seconds) {
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

    return 0.5f * sinf(2.0f * PI * row_freq[r] * t_seconds) +
           0.5f * sinf(2.0f * PI * col_freq[c] * t_seconds);
}

/* Detectar tecla en el buffer actual de muestras */
char detect_dtmf_in_window(const float *samples) {
    float row_w[4], col_w[4];
    float row_mag[4], col_mag[4];

    for (int i = 0; i < 4; i++) {
        row_w[i] = goertzel_omega(ROW_FREQ[i], FS);
        col_w[i] = goertzel_omega(COL_FREQ[i], FS);
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

#if USE_MICROPHONE
/* Lee N muestras del ADC a ~8 kHz usando micros() para sincronizar */
void read_microphone_window(float *buf, int n) {
    unsigned long interval_us = 1000000UL / (unsigned long)FS;  /* 125 us */
    for (int i = 0; i < n; i++) {
        unsigned long start_us = micros();
        int raw = analogRead(A0);
        /* Centrar ADC (0-1023) en rango [-1, 1]: restar 512 y escalar */
        buf[i] = ((float)raw - 512.0f) / 512.0f;
        /* Esperar al siguiente slot de 125 us */
        unsigned long elapsed = micros() - start_us;
        if (elapsed < interval_us) {
            delayMicroseconds(interval_us - elapsed);
        }
    }
}
#endif

void setup() {
    Serial.begin(9600);
    while (!Serial) { /* esperar */ }
    Serial.println(F("DTMF decoder con Goertzel (float)"));
#if USE_MICROPHONE
    Serial.println(F("Modo: micrófono en A0"));
#else
    Serial.println(F("Modo: tono simulado"));
#endif
    Serial.println(F("Esperando tonos..."));
}

char last_key = 0;
unsigned long tone_start = 0;

void loop() {
#if USE_MICROPHONE
    /* Lee una ventana completa del micrófono */
    read_microphone_window(samples, N);
    sample_idx = N;
#else
    /* Simular recepción de una tecla DTMF cada 3 segundos */
    static const char *keys = "123456789*0#ABCD";
    static int key_idx = 0;
    static unsigned long last_tone = 0;

    unsigned long now = millis();

    if (now - last_tone > 3000) {
        key_idx = (key_idx + 1) % 16;
        last_tone = now;
        tone_start = now;
        Serial.print(F("Enviando tecla: "));
        Serial.println(keys[key_idx]);
    }

    /* Generar N muestras del tono actual con tiempo en segundos de punto flotante */
    unsigned long t_elapsed = now - tone_start;
    if (t_elapsed < 1500) {  /* tono activo durante 1.5 s */
        char key = keys[(key_idx + 15) % 16];  /* tecla que se está enviando */
        float t0 = (float)t_elapsed / 1000.0f;  /* segundos de inicio de ventana */
        float dt = 1.0f / FS;                   /* 125 us por muestra */
        for (int i = 0; i < N; i++) {
            samples[i] = generate_dtmf_tone(key, t0 + (float)i * dt);
        }
        sample_idx = N;
#endif

        /* Detectar */
        char detected = detect_dtmf_in_window(samples);
        if (detected && detected != last_key) {
            Serial.print(F("Tecla detectada: "));
            Serial.println(detected);
            last_key = detected;
        } else if (!detected) {
            last_key = 0;
        }
#if USE_MICROPHONE
        /* Pequeña pausa entre ventanas para no saturar la CPU */
        delay(10);
#endif
    }

    delay(20);
}
