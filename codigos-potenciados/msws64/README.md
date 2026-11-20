# msws64 - Generador PRNG de 64 bits (multiplicación + xorshift)

> **Corrección (2026-11-20).** Este módulo **no** implementa el `msws64` de
> Widynski (2017), aunque así se atribuía antes en el README y en la
> cabecera `msws64.h`. El nombre del módulo y su API se conservan por
> compatibilidad con `lcg_lehmer/`, `hybrid_prng/` y `middle_square/`, pero la
> atribución era incorrecta y ya no debe repetirse.
>
> El `msws64` de Widynski es un xorshift de 64 bits con desplazamientos
> 12/25/27 y multiplicación final por `0x2545F4914F6CDD1D`. Su implementación
> canónica está ahora en `msws64_widynski.h` (este mismo directorio), y el
> test `msws64.c` muestra que **las dos salidas difieren desde la primera
> iteración**.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `msws64.h` | Cabecera con `msws64_t`, `msws64_seed`, `msws64_next`, `msws64_next_double`, `msws64_next_bounded` |
| `msws64_widynski.h` | Implementación canónica del msws64 de Widynski (solo como referencia para comparar) |
| `msws64.c` | Test comparativo: middle-square original, referencia de Widynski y χ² básico |
| `README.md` | Este archivo |

## Fórmulas

El generador realmente implementado usa el estado de 64 bits `s` y aplica:

```
s = s * 6364136223846793005
s ^= s >> 17
s = s * 1442695040888963407
s ^= s >> 31
s = s * 3141592653589793239
s ^= s >> 27
salida = s
```

La salida **es** el estado (no hay multiplicación final de salida como en el
algoritmo de Widynski).

El `msws64` de Widynski (`msws64_widynski.h`) es:

```
x = estado
x ^= x >> 12
x ^= x << 25
x ^= x >> 27
estado = x
salida = x * 0x2545F4914F6CDD1D
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
#include "msws64.h"

msws64_t rng;
msws64_seed(&rng, 1234ULL);  /* semilla no nula */

uint64_t r = msws64_next(&rng);      /* entero de 64 bits */
double d = msws64_next_double(&rng); /* double en [0, 1) */
uint64_t b = msws64_next_bounded(&rng, 100);  /* entero en [0, 100) */
```

## Compilar y ejecutar

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic msws64.c -o msws64_test -lm
./msws64_test
```

## Comparativa (resultados del test incluido)

| Generador | Ciclos (4 dígitos) | Zero sink | χ² (10 bins, 100k) | Velocidad |
|-----------|-------------------|-----------|------------------|-----------|
| middle-square | 8 | 18.18% | muy alto (sesgado) | lento (sprintf) |
| este módulo | — (estado 64 bits) | evitado por siembra | 10.99 | ~1.8 ns/iteración |

## Integración con el resto del proyecto

- `codigos-potenciados/goertzel/` — algoritmo de Goertzel con formulaciones K, α, β, híbrida de 3 zonas
- `codigos-potenciados/matematica_ancestral/` — utilidades matemáticas heredadas
- `codigos-potenciados/randu_plus.c` — RANDU + fmix32 (ejemplo histórico, no recomendado; ver la advertencia en el archivo)
- `codigos-potenciados/sine_fixed.c` — generadores de seno en punto fijo Q16.16 / Q32.32

## Referencias

- M. Widynski, "Middle Square Weyl Sequence PRNG" (2017) — algoritmo del
  fichero `msws64_widynski.h` (este módulo **no** lo implementa).