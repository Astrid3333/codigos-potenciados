# msws64 - Generador PRNG Widynski (2017)

Implementación en C del generador de números pseudoaleatorios **msws64** de
Widynski (Middle Square Weyl Sequence de 64 bits), una alternativa moderna
y rápida al middle-square de von Neumann.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `msws64.h` | Cabecera con `msws64_t`, `msws64_seed`, `msws64_next`, `msws64_next_double`, `msws64_next_bounded` |
| `msws64.c` | Implementación completa + test comparativo contra middle-square original |
| `README.md` | Este archivo |

## Formulas

El generador usa el estado de 64 bits `s` y aplica:

```
s = s * 6364136223846793005
s ^= s >> 17
s = s * 1442695040888963407
s ^= s >> 31
s = s * 3141592653589793239
s ^= s >> 27
```

## Uso

```c
#include "msws64.h"

msws64_t rng;
msws64_seed(&rng, 1234ULL);  /* semilla no nula */

uint64_t r = msws64_next(&rng);      /* entero de 64 bits */
double d = msws64_next_double(&rng); /* double en [0, 1) */
uint64_t b = msws64_next_bounded(&rng, 100);  /* entero en [0, 100) */
```

## Compilar y ejecutar

```bash
gcc -std=c11 -O2 msws64.c -o msws64_test -lm
./msws64_test
```

## Comparativa (resultados del test incluido)

| Generador | Ciclos (4 dígitos) | Zero sink | Chi² (10 bins) | Velocidad |
|-----------|-------------------|-----------|----------------|-----------|
| middle-square | 8 | 18.18% | muy alto (sesgado) | lento (sprintf) |
| msws64 | ~2^64 (teórico) | ~0% | ~9 (esperado) | ~1.8 ns/iteración |

## Integración con el resto del proyecto

- `codigos-potenciados/goertzel/` — algoritmo de Goertzel con formulaciones K, α, β, híbrida
- `codigos-potenciados/randu_plus.c` — RANDU + fmix32
- `codigos-potenciados/sine_fixed.c` — generadores de seno en punto fijo

## Referencia

M. Widynski, "Middle Square Weyl Sequence PRNG" (2017).