# mult_xorshift64 - Generador PRNG de 64 bits (multiplicación + xorshift)

> **Renombrado (2026-11-20).** Este módulo se llamaba `msws64` y se atribuía a
> Widynski (2017). **Ambas cosas eran falsas**: el nombre y la atribución.
>
> - Lo implementado es una **cadena de multiplicación-xorshift de 3 rondas**,
>   cuyo nombre correcto es `mult_xorshift64`.
> - El **msws64 de Widynski** (2017) es otro algoritmo: elevar el estado al
>   cuadrado, sumarle un incremento de Weyl acumulado y rotar 32 bits. Ahora
>   está implementado en el módulo `../msws_widynski/` (`msws_widynski.h`,
>   con sus propias pruebas contra fuentes externas) y se usa aquí como
>   referencia para comparar.
> - El **xorshift64\* de Vigna** (desplazamientos 12/25/27 con multiplicador
>   final `0x2545F4914F6CDD1D`) es un tercer generador, distinto de ambos.
>
> Rutas antiguas (`codigos-potenciados/msws64/msws64.h`) y símbolos
> (`msws64_t`, `msws64_next`, …) ya no existen: actualiza los `#include` y las
> llamadas a `mult_xorshift64.h` / `mult_xorshift64_next()`.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `mult_xorshift64.h` | Cabecera con `mult_xorshift64_t`, `mult_xorshift64_seed`, `mult_xorshift64_next`, `mult_xorshift64_next_double`, `mult_xorshift64_next_bounded` |
| `mult_xorshift64.c` | Test comparativo: middle-square original, referencia de Widynski (`../msws_widynski/`) y χ² básico |
| `mult_xorshift64_period_test.c` | Medición del periodo (Brent), comprobación de biyectividad por inversa explícita y barrido de puntos fijos |
| `README.md` | Este archivo |

## Fórmulas

El generador implementado usa el estado de 64 bits `s` y aplica:

```
s = s * 6364136223846793005
s ^= s >> 17
s = s * 1442695040888963407
s ^= s >> 31
s = s * 3141592653589793239
s ^= s >> 27
salida = s
```

La salida **es** el estado (no hay multiplicación final de salida).

El msws64 de Widynski (módulo `../msws_widynski/`) es:

```
x = x * x                 (cuadrado)
w = w + INCREMENTO_WEYL   (incremento acumulado)
x = x + w
x = rotl(x, 32)
salida = x
```

## Propiedades que sí se pueden afirmar

| Propiedad | Estado |
|-----------|--------|
| Estado | 64 bits (`uint64_t`) |
| Periodo | **Cota inferior verificada:** λ > 2³⁷/3 ≈ 4.58·10¹⁰ (> 2³⁵), medida con Brent en tres semillas sin detección de ciclo en 2³⁷ pasos. El periodo exacto (orden 2⁶⁴) no es alcanzable por medición directa |
| Preperiodo | Ninguno: cada paso es una composición de seis biyecciones de Z/2⁶⁴ (tres multiplicaciones por constantes impares y tres xorshift), luego el mapa de estado es una biyección y toda órbita es un ciclo puro |
| Biyectividad | Verificada: la inversa explícita (unxorshift-right 27/31/17 + inversos modulares por Newton) satisface `M⁻¹(M(x)) = x` en 2.000.000 de valores, 0 fallos |
| Sumidero 0 | La semilla 0 quedaría fija en 0; la siembra fuerza estado impar (`seed \| 1`). En 2.000.000 de muestras no se encontró ningún otro punto fijo |
| χ² (10 bins, 100 000 muestras, semilla 1234) | 10.99 (9 g.l.; umbral 95% = 16.92) |
| SmallCrush (TestU01 1.2.3) | 22 de 24 configuraciones (3 modos de troceado × 8 semillas) sin ninguna estadística fuera de rango. Las 2 excepciones son fallos **marginales de una sola estadística** (p = 0.9991, a 0.0001 del umbral 0.9990), compatibles con el ~0.7 de falsos positivos esperados en 360 comprobaciones |

El χ² con 10 bins y 100 000 muestras es una comprobación **débil**: no tiene
potencia contra sesgos de grano fino ni contra correlaciones. Las mediciones
con tests reales (7 tests × 8 semillas, controles limpio y defectuoso,
p-valores exactos) están en `VALIDATION_REPORT_PRNG_MODULES.md` en la raíz
del repositorio, generadas por `prng_modules_validator.py`. Las baterías de
TestU01 sobre este generador (SmallCrush en los tres modos de troceado,
junto con `hybrid_prng` y `xoshiro256**` como referencia) están en
`VALIDATION_REPORT.md`, generadas por `randu_plus_validator.py` con el
arnés `codigos-potenciados/testu01/run_prng_modules`.

## Uso

```c
#include "mult_xorshift64.h"

mult_xorshift64_t rng;
mult_xorshift64_seed(&rng, 1234ULL);  /* semilla no nula */

uint64_t r = mult_xorshift64_next(&rng);      /* entero de 64 bits */
double d = mult_xorshift64_next_double(&rng); /* double en [0, 1) */
uint64_t b = mult_xorshift64_next_bounded(&rng, 100);  /* entero en [0, 100) */
```

## Compilar y ejecutar

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic mult_xorshift64.c -o mult_xorshift64_test -lm
./mult_xorshift64_test
```

Periodo y biyectividad (medición, no solo teoría):

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic mult_xorshift64_period_test.c -o mult_xorshift64_period_test
./mult_xorshift64_period_test                  # límite 2^28 (segundos)
./mult_xorshift64_period_test 137438953472     # límite 2^37 (~12 min por semilla)
```

## Comparativa (resultados del test incluido)

| Generador | Ciclos (4 dígitos) | Zero sink | χ² (10 bins, 100k) | Velocidad |
|-----------|-------------------|-----------|------------------|-----------|
| middle-square | 8 | 18.18% | muy alto (sesgado) | lento (sprintf) |
| este módulo | λ > 4.58·10¹⁰ (medido, sin preperiodo) | evitado por siembra | 10.99 | ~1.8 ns/iteración |

## Integración con el resto del proyecto

- `codigos-potenciados/lcg_lehmer/` — híbrido LCG + generador de 64 bits (usa este generador)
- `codigos-potenciados/hybrid_prng/` — híbrido middle-square + generador de 64 bits
- `codigos-potenciados/matematica_ancestral/` — utilidades matemáticas heredadas
- `codigos-potenciados/middle_square/` — middle-square de von Neumann (referencia histórica)
- `codigos-potenciados/randu_plus.c` — RANDU + fmix32 (ejemplo histórico, no recomendado; ver la advertencia en el archivo)

## Referencias

- M. Widynski, "Middle Square Weyl Sequence PRNG" (2017) — algoritmo del
  módulo `../msws_widynski/` (este módulo **no** lo implementa).
- D. Blackman / S. Vigna, "Xorshift RNGs" (2014) — `xorshift64*`, tercer
  generador distinto, mentioned aquí solo para evitar confusiones.
