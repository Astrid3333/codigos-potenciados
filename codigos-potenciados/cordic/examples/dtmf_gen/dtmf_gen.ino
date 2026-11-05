/*
 * DTMF generator con CORDIC en punto fijo Q15 para Arduino.
 *
 * Genera tonos DTMF (dual-tone multi-frequency) usando CORDIC para calcular
 * seno y coseno sin librería matemática (sin fp, sin sin/cos de libm).
 *
 * Cada tecla DTMF es la suma de dos senos: una frecuencia de fila y una de
 * columna. El algoritmo CORDIC calcula ambos senos usando solo sumas,
 * restas y desplazamientos (shifts).
 *
 * Configuración:
 *   fs = 8000 Hz  (frecuencia de muestreo)
 *   N  = 205      (muestras por tono, ~25.6 ms)
 *
 * Frecuencias DTMF:
 *   Filas:    697, 770, 852, 941 Hz
 *   Columnas: 1209, 1336, 1477, 1633 Hz
 *
 * Salida: PWM en pin 9 (o DAC si lo tiene la placa) a 8 kHz.
 * Para escuchar: conectar un altavoz con resistencia en serie al pin 9.
 */

#include <cordic_arduino.h>

#define FS          8000.0f   /* frecuencia de muestreo (Hz) */
#define N           205       /* muestras por ventana (25.6 ms @ 8kHz) */
#define OUTPUT_PIN  9         /* pin de salida PWM/DAC */

/* Tabla de frecuencias DTMF */
static const float ROW_FREQ[4] = {697.0f, 770.0f, 852.0f, 941.0f};
static const float COL_FREQ[4] = {1209.0f, 1336.0f, 1477.0f, 1633.0f};

/* Tabla de teclas DTMF */
static const char KEYS[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* Estado del generador */
static cordic_fixed16_t row_phase[4];   /* fase acumulada para cada fila */
static cordic_fixed16_t col_phase[4];   /* fase acumulada para cada columna */
static bool initialized = false;

/* Inicializar fases para todas las teclas */
static void dtmf_init(void) {
    for (int i = 0; i < 4; i++) {
        row_phase[i] = cordic_radians_to_fixed16(0.0);
        col_phase[i] = cordic_radians_to_fixed16(0.0);
    }
    initialized = true;
}

/* Generar una muestra de tono DTMF para la tecla dada.
 * Avanza las fases de la fila y columna correspondientes. */
static cordic_fixed16_t dtmf_next_sample(char key) {
    int r = -1, c = -1;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (KEYS[i][j] == key) { r = i; c = j; }
        }
    }
    if (r < 0 || c < 0) return 0;

    /* Calcular senos de fila y columna */
    cordic_fixed16_t row_sin = cordic_sin_fixed16(row_phase[r]);
    cordic_fixed16_t col_sin = cordic_sin_fixed16(col_phase[c]);

    /* Avanzar fases para la próxima muestra */
    row_phase[r] += cordic_radians_to_fixed16(2.0 * PI * ROW_FREQ[r] / FS);
    col_phase[c] += cordic_radians_to_fixed16(2.0 * PI * COL_FREQ[c] / FS);

    /* Mezclar ambos tonos (mitad cada uno) */
    return (cordic_fixed16_t)((row_sin + col_sin) / 2);
}

/* Escribir muestra al DAC/PWM */
static inline void output_sample(cordic_fixed16_t sample) {
    /* Mapear Q15 [-32768, 32767] a [0, 255] para PWM de 8 bits */
    int val = ((int)sample + 32768) >> 8;
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    analogWrite(OUTPUT_PIN, (uint8_t)val);
}

void setup() {
    Serial.begin(9600);
    while (!Serial) { /* esperar */ }

    pinMode(OUTPUT_PIN, OUTPUT);

    dtmf_init();

    Serial.println(F("DTMF generator con CORDIC (float point Q15)"));
    Serial.println(F("Tono: 123456789*0#ABCD, 1.5s cada tecla"));
    Serial.println(F("Conectar altavoz al pin 9 (o DAC)"));
    Serial.println();
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
        current_key = keys[key_idx];
        key_idx = (key_idx + 1) % 16;
        last_tone = now;
        tone_start = now;
        Serial.print(F("Tono: "));
        Serial.println(current_key);
    }

    /* Generar N muestras del tono actual */
    unsigned long t_elapsed = now - tone_start;
    if (t_elapsed < 1500) {  /* tono activo durante 1.5 s */
        char key = current_key;  /* misma tecla que se anunció */

        for (int i = 0; i < N; i++) {
            cordic_fixed16_t sample = dtmf_next_sample(key);
            output_sample(sample);
            /* Esperar ~125 us entre muestras (8 kHz) */
            delayMicroseconds(125);
        }
    } else {
        /* Silencio entre tonos */
        output_sample(0);
        delayMicroseconds(125);
    }
}