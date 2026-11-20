# mult_xorshift64 - Generador PRNG de 64 bits (multiplicación + xorshift)

> **Renombrado (2026-11-20).** Este módulo se llamaba `msws64` y se atribuía a
> Widynski (2017). **Ambas cosas eran falsas**: el nombre y la atribución.
>
> - Lo implementado es una **cadena de multiplicación-xorshift de 3 rondas**,
>   cuyo nombre correcto es `mult_xorshift64`.
> - El **msws64 de Widynski** (2017) es otro algoritmo: elevar el estado al
>   cuadrado, sumarle un incremento de Weyl acumulado y rotar 32 bits. Su
>   estructura está en `msws64_widynski.h` (este mismo directorio), tal como
>   la usa la implementación en Octave de `octave-mcp`
>   (`middle_square_weyl_tool.py`).
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
| `msws64_widynski.h` | msws64 de Widynski (cuadrado + Weyl + rotación), solo como referencia para comparar |
| `mult_xorshift64.c` | Test comparativo: middle-square original, referencia de Widynski y χ² básico |
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

El msws64 de Widynski (`msws64_widynski.h`) es:

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
| Periodo | **No verificado.** No es un LCG con orden conocido; solo se comprobó que no repite en las primeras 100 000 muestras |
| Sumidero 0 | La semilla 0 quedaría fija en 0; la siembra fuerza estado impar (`seed \| 1`) |
| χ² (10 bins, 100 000 muestras, semilla 1234) | 10.99 (9 g.l.; umbral 95% = 16.92) |

El χ² con 10 bins y 100 000 muestras es una comprobación **débil**: no tiene
potencia contra sesgos de grano fino ni contra correlaciones. Las mediciones
con tests reales (7 tests × 8 semillas, controles limpio y defectuoso,
p-valores exactos) están en `VALIDATION_REPORT_PRNG_MODULES.md` en la raíz
del repositorio, generadas por `prng_modules_validator.py`.

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

## Comparativa (resultados del test incluido)

| Generador | Ciclos (4 dígitos) | Zero sink | χ² (10 bins, 100k) | Velocidad |
|-----------|-------------------|-----------|------------------|-----------|
| middle-square | 8 | 18.18% | muy alto (sesgado) | lento (sprintf) |
| este módulo | — (estado 64 bits) | evitado por siembra | 10.99 | ~1.8 ns/iteración |

## Integración con el resto del proyecto

- `codigos-potenciados/lcg_lehmer/` — híbrido LCG + generador de 64 bits (usa este generador)
- `codigos-potenciados/hybrid_prng/` — híbrido middle-square + generador de 64 bits
- `codigos-potenciados/matematica_ancestral/` — utilidades matemáticas heredadas
- `codigos-potenciados/middle_square/` — middle-square de von Neumann (referencia histórica)
- `codigos-potenciados/randu_plus.c` — RANDU + fmix32 (ejemplo histórico, no recomendado; ver la advertencia en el archivo)

## Referencias

- M. Widynski, "Middle Square Weyl Sequence PRNG" (2017) — algoritmo del
  fichero `msws64_widynski.h` (este módulo **no** lo implementa).
- D. Blackman / S. Vigna, "Xorshift RNGs" (2014) — `xorshift64*`, tercer
  generador distinto, mentioned aquí solo para evitar confusiones.
