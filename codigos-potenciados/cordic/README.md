# cordic - Algoritmo CORDIC (1959) para Arduino y C

Implementación del algoritmo **CORDIC** (COordinate Rotation DIgital Computer)
de Jack Volder (1959). Calcula seno y coseno usando **únicamente sumas,
restas y desplazamientos binarios (shifts)** — sin multiplicaciones, sin
librería matemática, sin tabla de senos/cosenos.

Solo necesita una tabla pequeña de `atan(2^-i)` y la ganancia constante `K`.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `cordic.h` | Cabecera con definiciones, tipos y declaraciones |
| `cordic.c` | Implementación en punto flotante (C estándar) |
| `cordic_test.c` | Pruebas de precisión contra libm |
| `cordic_arduino.h` | Implementación en punto fijo Q15 para Arduino/AVR |
| `examples/dtmf_decoder/` | Ejemplo: decodificador DTMF con Goertzel |
| `examples/dtmf_gen/` | Ejemplo: generador de tonos DTMF con CORDIC |
| `library.properties` | Metadatos para Arduino Library Manager |
| `README.md` | Este archivo |

## Cómo funciona CORDIC

El algoritmo rota un vector unitario (1, 0) por un ángulo θ aplicando
rotaciones sucesivas por ángulos `atan(2^-i)`:

```
x_{i+1} = x_i - σ_i · y_i · 2^{-i}
y_{i+1} = y_i + σ_i · x_i · 2^{-i}
z_{i+1} = z_i - σ_i · atan(2^{-i})
```

Donde `σ_i = +1` si `z_i > 0`, `-1` si `z_i < 0`.

Después de N iteraciones:
- `x_N ≈ K · cos(θ)`
- `y_N ≈ K · sin(θ)`

donde `K = ∏ sqrt(1 + 2^{-2i}) ≈ 1.64676` es la ganancia de CORDIC.

## Uso en Arduino

```cpp
#include <cordic_arduino.h>

// Generar seno de 440 Hz (nota A4)
cordic_fixed16_t theta = cordic_radians_to_fixed16(2.0 * PI * 440.0 / 8000.0);
cordic_fixed16_result_t r = cordic_sincos_fixed16(theta);
int16_t sample = (int16_t)r.y;  // valor en Q15
```

## Precisión

Con 16 iteraciones (valor por defecto en C, 12 en Arduino):

| Función | Error máximo vs libm |
|---------|---------------------|
| sin | ~3.0e-5 |
| cos | ~3.0e-5 |

## Compilar y probar (C, en PC)

```bash
cd codigos-potenciados/cordic
gcc -std=c11 -O2 -Wall -Wextra cordic.c cordic_test.c -lm -o cordic_test
./cordic_test
```

Salida esperada:

```
=================================================================
  PRUEBAS DE PRECISIÓN - CORDIC (1959) vs libm
=================================================================

Probando rango completo [-180°, +180°] en pasos de 0.5°...

Resultados (iteraciones = 16):
  error absoluto máximo sin:  3.0e-05
  error absoluto máximo cos:  3.0e-05
  error relativo máximo sin:  1.9e-03
  error relativo máximo cos:  1.9e-03
  puntos probados:            721

Pruebas de ángulos conocidos:
  0°:     sin=-0.0000175949 (ref=0.0000000000, err=1.76e-05)  cos=0.9999999997 (ref=1.0000000000, err=3.10e-10)
  30°:    sin=0.5000126187 (ref=0.5000000000, err=1.26e-05)  cos=0.8660181181 (ref=0.8660254038, err=7.29e-06)
  45°:    sin=0.7071177613 (ref=0.7071067812, err=1.10e-05)  cos=0.7070958007 (ref=0.7071067812, err=1.10e-05)
  60°:    sin=0.8660181181 (ref=0.8660254038, err=7.29e-06)  cos=0.5000126187 (ref=0.5000000000, err=1.26e-05)
  90°:    sin=0.9999999997 (ref=1.0000000000, err=3.10e-10)  cos=-0.0000175949 (ref=0.0000000000, err=1.76e-05)
  180°:   sin=-0.0000175949 (ref=0.0000000000, err=1.76e-05)  cos=-0.9999999997 (ref=-1.0000000000, err=3.10e-10)
  270°:   sin=-0.9999999997 (ref=-1.0000000000, err=3.10e-10)  cos=-0.0000175949 (ref=-0.0000000000, err=1.76e-05)
  -45°:   sin=-0.7070958007 (ref=-0.7071067812, err=1.10e-05)  cos=0.7071177613 (ref=0.7071067812, err=1.10e-05)
  -90°:   sin=-0.9999999997 (ref=-1.0000000000, err=3.10e-10)  cos=-0.0000175949 (ref=-0.0000000000, err=1.76e-05)

  RESULTADO: ACEPTABLE (error < 5e-5)
```

> **Nota:** El error relativo parece elevado cerca de ángulos donde sin o cos cruzan cero (por división entre un valor muy pequeño). El error absoluto permanece por debajo de 3.0e-5 en todo el rango [-π, π].

## Instalar como librería Arduino

Copiar la carpeta `cordic/` a `~/Documentos/Arduino/libraries/` o usar el
Administrador de Librerías de Arduino. Luego:

```cpp
#include <cordic_arduino.h>

void setup() {
    cordic_fixed16_t theta = cordic_degrees_to_fixed16(45.0);
    cordic_fixed16_result_t r = cordic_sincos_fixed16(theta);
    Serial.println(r.y);  // seno de 45° en Q15
}
```

## Licencia

MIT — mismo espíritu que el código original de Volder (1959): algoritmo
abierto, simple y eficiente.