# hybrid_prng - Generador PRNG híbrido moderno (C11)

Combina **middle-square de von Neumann** (componente histórico, 4 dígitos) con
el **generador de 64 bits del módulo `mult_xorshift64/`** (cadena de
multiplicación-xorshift de 3 rondas, fuente principal de entropía).

> **Corrección (2026-11-20).** Antes este README (y la cabecera) atribuían el
> generador de 64 bits a Widynski (2017). **No lo es**: el msws64 de Widynski
> es un generador de **cuadrado + incremento de Weyl + rotación de 32 bits**,
> implementado en el módulo `../msws_widynski/`. Las dos implementaciones
> difieren desde la primera iteración.

## ¿Por qué un híbrido?

Middle-square de von Neumann (1946) está **roto como PRNG independiente**:
- Solo 8 ciclos posibles en 4 dígitos
- 18.18% de las semillas colapsan a cero
- Ciclos muy cortos

El generador de 64 bits es un PRNG decente de 64 bits por sí solo.

El híbrido **XOR** de ambos produce una secuencia con buena distribución
estadística sin depender exclusivamente de un solo mecanismo. middle-square
aporta estructura determinista predecible; el generador de 64 bits aporta
entropía de 64 bits.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `hybrid_prng.h` | Cabecera con `hybrid_prng_t`, `hybrid_prng_seed`, `hybrid_prng_next`, `hybrid_prng_next_double`, `hybrid_prng_next_bounded` |
| `hybrid_prng.c` | Implementación completa + test estadístico comparativo |
| `hybrid_prng_test.c` | Test unitario rápido (semillas, reproducibilidad, rango, double) |
| `README.md` | Este archivo |

## Uso

```c
#include "hybrid_prng.h"

hybrid_prng_t rng;
hybrid_prng_seed(&rng, 1234ULL);  /* semilla no nula */

uint64_t r = hybrid_prng_next(&rng);        /* entero de 64 bits */
double d = hybrid_prng_next_double(&rng);   /* double en [0, 1)   */
uint64_t b = hybrid_prng_next_bounded(&rng, 100); /* entero en [0, 100) */
```

## Compilar y ejecutar

```bash
gcc -std=c11 -O2 hybrid_prng.c -o hybrid_prng -lm
./hybrid_prng

gcc -std=c11 -O2 hybrid_prng_test.c hybrid_prng.c -o hybrid_prng_test
./hybrid_prng_test
```

## Resultados esperados

```
=== hybrid_prng (middle-square + mult_xorshift64) - Test ===

Middle-square original (4 digitos):
  Ciclos: 8
  Zero sink: 1818 de 10000 (18.18%)
  Estado: ROTADO como PRNG independiente

Hybrid PRNG (middle-square + mult_xorshift64):
  Semilla: 1234
  Muestras: 100000
  Chi^2 (10 bins): ~10.0 (esperado ~9 para uniformidad)
  Media uint64: ~4611686018427387904 (esperado ~2^63)
  Media double: ~0.5

Primeros 10 valores (hibrido):
   1: uint64=..., double=0.xxxxxx
   ...

Uso practico: generar IDs de 64 bits (ej. IDs de transaccion):
  ID 1: XXXXXXXXXXXXXXXX
  ...
```

## Relación con `mult_xorshift64` (medido, no supuesto)

La mezcla middle-square solo ocupa los **bits 32-63** de la salida, así que la
**mitad baja coincide palabra a palabra** con la de `mult_xorshift64`
sembrado igual. Medido con el arnés TestU01 (digest FNV-1a de 1.000.000 de
salidas de 32 bits, `codigos-potenciados/testu01/run_prng_modules digest`):

| Modo de troceado | `mult_xorshift64` | `hybrid_prng` | ¿Mismo flujo? |
|---|---|---|---|
| `low` (mitad baja) | `1d63fbd9ce5edc96` | `1d63fbd9ce5edc96` | **Sí, idéntico** |
| `high` (mitad alta) | `e9fcb764c18a2c2f` | `0d340876c046d06b` | No |
| `split` (ambas) | `cb18d4e799d20df4` | `fce7a59bc8f6e7e4` | No |

Consecuencia práctica: medir `hybrid_prng` en modo `low` **no** es una
medición independiente de `mult_xorshift64` en modo `low`. Para una
comprobación independiente hay que usar `high` o `split`.

## Integración con el resto del proyecto

- `codigos-potenciados/mult_xorshift64/` — generador de 64 bits puro (componente del híbrido)
- `codigos-potenciados/msws_widynski/` — msws64 de Widynski real (cuadrado + Weyl + rotación)
- `codigos-potenciados/goertzel/` — algoritmo Goertzel con formulaciones K, α, β, híbrida
- `codigos-potenciados/randu_plus.c` — RANDU + fmix32

## Limitaciones

middle-square **original** sigue roto como PRNG independiente. Este híbrido
no "arregla" middle-square; lo usa como componente mezclado con el generador
de 64 bits.
Para un PRNG puro y usable, usa el módulo `codigos-potenciados/mult_xorshift64/`.

## Referencia

- J. von Neumann, "Various techniques used in connection with random digits" (1946/1951)
- M. Widynski, "Middle Square Weyl Sequence PRNG" (2017) — **referencia del
  msws64 canónico**, implementado en `../msws_widynski/` (este módulo no lo
  usa; ver la corrección arriba)