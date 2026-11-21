# MSWS de Widynski — Middle Square Weyl Sequence

Generadores pseudoaleatorios de la familia **MSWS** (Middle Square Weyl
Sequence), publicada por **Bernard Widynski** en 2017.

## Qué es y qué no es

El estado del generador se **eleva al cuadrado**, se le suma el incremento
acumulado de una **secuencia de Weyl** y se **rotan 32 bits** dentro de la
palabra de 64 bits:

```
x  = x*x
w += s            (secuencia de Weyl: w(i) = i*s mod 2^64)
x  = x + w
x  = rot32(x)     (intercambio de las dos mitades de 64 bits)
```

> **No es un xorshift.** El `xorshift64*` de Vigna (desplazamientos 12/25/27 con
> multiplicador final `0x2545F4914F6CDD1D`) es **otro** generador, con otro
> origen y otro comportamiento. El MSWS usa exponenciación y suma de Weyl.

## Variantes

| Tipo | Salida | Descripción |
|---|---|---|
| `msws32_widynski_t` | 32 bits | Salida oficial del artículo (sección 2) |
| `msws64_widynski_t` | 64 bits | Dos carriles independientes (ILP), sección 8 del artículo |
| `msws64_mono_widynski_t` | 64 bits | Un solo carril con la palabra completa |

## API

```c
#include "msws_widynski.h"

msws32_widynski_t     r32;
msws64_widynski_t     r64;
msws64_mono_widynski_t rmono;

/* siembra */
msws32_widynski_seed(&r32, MSWS32_WIDYNSKI_S_PAPER);
msws64_widynski_seed(&r64, MSWS64_WIDYNSKI_S1_PAPER,
                      MSWS64_WIDYNSKI_S2_PAPER);
msws64_mono_widynski_seed(&rmono, 20261120ULL);   /* semilla entera cualquiera */

/* siembra para flujos paralelos: x = w = s */
msws32_widynski_seed_stream(&r32, s);
msws64_widynski_seed_stream(&r64, s1, s2);

/* salida */
uint32_t a = msws32_widynski_next(&r32);
uint64_t b = msws64_widynski_next(&r64);
uint64_t c = msws64_mono_widynski_next(&rmono);

/* ayudas */
double   d = msws32_widynski_next_double(&r32);      /* [0, 1), 32 bits */
double   e = msws64_widynski_next_double(&r64);       /* [0, 1), 53 bits */
uint32_t f = msws32_widynski_next_bounded(&r32, 100); /* [0, 100) sin sesgo */
uint64_t g = msws64_widynski_next_bounded(&r64, 1000000007ULL);

/* salto adelante (flujos disjuntos) */
msws32_widynski_jump_ahead(&r32, 1000000000ULL);
msws64_widynski_jump_ahead(&r64, 1000000000ULL);
msws64_mono_widynski_jump_ahead(&rmono, 1000000000ULL);
```

## Constantes Weyl publicadas

```c
#define MSWS32_WIDYNSKI_S_PAPER  0xB5AD4ECEDA1CE2A9ULL
#define MSWS64_WIDYNSKI_S1_PAPER 0xB5AD4ECEDA1CE2A9ULL
#define MSWS64_WIDYNSKI_S2_PAPER 0x278C5A4D8419FE6BULL
```

El incremento `s` debe ser **impar** (Teorema A del artículo: la secuencia de
Weyl tiene período 2⁶⁴ solo si `s` es impar y coprimo con 2⁶⁴), por lo que las
funciones de siembra fuerzan el bit menos significativo con `| 1`.

## Verificación

Las pruebas **no** comparan contra la salida de este mismo módulo: las
secuencias de referencia se congelaron desde implementaciones **externas e
independientes**:

| Fuente | Licencia | Qué aporta |
|---|---|---|
| Implementación de referencia del autor, v5 (`mswsrngv5`: `msws32.h`, `msws64.h`) | GPL v3 | Oráculo canónico de `msws32` y `msws64` (dos carriles) |
| Crate Rust [`twe4ked/msws`](https://crates.io/crates/msws) | sin LICENSE declarada | Segunda fuente para `msws32`: coincide **bit a bit** (12/12) con el autor |
| Crate Rust [`tidwall/weyl-rs`](https://crates.io/crates/weyl) | ISC | Oráculo de la variante mono de 64 bits (16/16 vectores) |

Además se comprueban invariantes internas, el ejemplo de la sección 3 del
artículo, las propiedades matemáticas del salto adelante, los rangos de las
funciones de ayuda y la uniformidad (chi-cuadrado).

### Compilar y ejecutar

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic msws_widynski_test.c -o prueba
./prueba
```

Resultado actual: **20/20 pruebas pasan**.

## Licencia y atribución

- **Artículo de referencia:** M. Widynski, *Middle Square Weyl Sequence RNG*,
  arXiv:1704.00358 (2017).
- **Autoría del código de este módulo:** reimplementación propia a partir de las
  fórmulas del artículo. **No** se ha copiado código de la implementación de
  referencia del autor (GPL v3) ni de ningún port derivado.
- Las **secuencias numéricas de prueba** usadas en `msws_widynski_test.c` son
  datos, no código: proceden de las fuentes externas listadas arriba.
- **`jump_ahead` es una reimplementación propia.** La rutina del autor solo está
  publicada en su cabecera GPLv3, así que aquí se reimplementa la idea desde la
  prosa del artículo (Teorema A: `w += m*s`) con una aleatorización del estado
  al estilo *counter-based squares*. **Consecuencia:** las secuencias con salto
  de este módulo **no** coinciden bit a bit con las del autor; lo que sí se
  garantiza es la propiedad matemática (el acumulador de Weyl queda exactamente
  `m*s` por delante) y la disyunción de los flujos.

El repositorio no incluye archivo `LICENSE`; el estado de la licencia del
proyecto está pendiente de definirse.
