# pi_ancestral - π por polígonos (Arquímedes) y extrapolación de Liu Hui

Implementación en C de las dos vías históricas para aproximar π sin
calculadora: el **perímetro del polígono circunscrito** que Arquímedes
duplicaba de 6 en 6 lados, y la **extrapolación de Liu Hui** (promedio de
perímetros + refinamiento de Richardson) que aplica **`R_j[k] = R_{j-1}[k+1] +
(R_{j-1}[k+1] - R_{j-1}[k]) / (4^j - 1)`** sobre esa misma secuencia.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `pi_ancestral.h` | Cabecera con tipos y declaraciones |
| `pi_ancestral.c` | Implementación (perímetros, cotas, extrapolación) |
| `pi_ancestral_test.c` | Pruebas numéricas y de convergencia |
| `README.md` | Este archivo |

## Uso

```c
#include "pi_ancestral.h"

double p = pi_arquimedes_estable(20);      /* perimetro n*t, recursion estable */
double q = pi_arquimedes_ingenuo(20);      /* misma recursion, se rompe */
double r = pi_liu_hui(20, 3);               /* extrapolacion de Liu Hui */
double inf, sup;
pi_arquimedes_cotas(10, &inf, &sup);       /* inf <= pi <= sup */
```

## Las dos recursiones del perímetro

Con `t = tan(π/n)` y `n = 6·2^k`, el perímetro del polígono circunscrito a
un círculo de radio 1 es `n·t` y baja hacia π desde arriba:

| Recursión | Fórmula | Comportamiento |
|-----------|---------|----------------|
| ingenua | `t = (sqrt(1+t²) - 1) / t` | resta dos números casi iguales: a `t < 1e-8` el numerador se anula y la secuencia colapsa (0 o NaN hacia k≈27) |
| estable | `t = t / (sqrt(1+t²) + 1)` | racionaliza la resta y conserva los dígitos: error ~4⁻ᵏ hasta el piso de redondeo de `double` (~3.6e-15) |

Medido en este módulo (doble precisión):

| k | error perímetro estable | error Liu Hui (3 niveles) |
|---|-------------------------|--------------------------|
| 4 | 1.1e-03 | 3.8e-10 |
| 10 | 2.7e-07 | 1.3e-15 |
| 20 | 2.6e-13 | 2.7e-15 |
| 30 | 3.6e-15 | 3.6e-15 |

## Cotas de Arquímedes

Con semiperímetros `i` (hexágono inscrito = 3) y `c` (hexágono circunscrito
= 2√3), al duplicar los lados:

```
c' = 2·i·c / (i + c)              media armónica  -> nuevo circunscrito
i' = i · sqrt(2·c / (i + c))                      -> nuevo inscrito
```

> **Nota importante**: el perímetro inscrito siguiente **no** es la media
> geométrica `sqrt(i·c)` de los valores viejos. La recurrencia correcta es
> `i' = i·sqrt(2·c/(i+c))` con `i` y `c` anteriores. El orden de las dos
> actualizaciones importa: `c' = 2·i·c/(i+c)` y después `i' = sqrt(c'·i)` sí
> son equivalentes —verificado: convergen a π con error 1.3e-15 a las 30
> iteraciones, que es justo lo que hace la herramienta `archimedes_pi` de
> `mcp-ecosystem-chiloe`—, pero si el inscrito se actualiza **antes** que el
> circunscrito la iteración colapsa a ≈3.300413165133028 en lugar de a π.
> El módulo usa la forma explícita con los valores viejos, con
> `i(10) = 3.141592516692157` y `c(10) = 3.141592927385096`.

## Compilar y probar

```bash
gcc -std=c11 -O2 -Wall -Wextra pi_ancestral.c pi_ancestral_test.c -lm -o prueba
./prueba
```

Compila limpio con `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Werror`.

## Referencias

- Arquímedes, *Medida de un círculo* (c. 250 a.C.), proposiciones II.2–II.3.
- Liu Hui, *Commentario sobre los nueve capítulos* (263 d.C.), cap. 7
  ("promedio de los perímetros de los polígonos").
- W. J. Churchhouse, *A Note on Pi, the Midpoint and the Doubling of the
  Polygon* (1964): la extrapolación `4^j - 1` sobre la secuencia de
  perímetros.
