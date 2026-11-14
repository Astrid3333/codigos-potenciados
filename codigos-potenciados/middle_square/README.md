# middle_square - Generador de von Neumann (middle square) de 4 dígitos

Implementación en C del método de **Von Neumann** (1946, "Various techniques
used in connection with random digits"), en su versión de **4 dígitos
decimales**: el estado se eleva al cuadrado y se conservan los cuatro dígitos
centrales del resultado.

```
x' = (x² / 100) mod 10000          (equivalente a tomar los dígitos 2..5 del
                                     cuadrado escrito con 8 dígitos, con ceros
                                     a la izquierda)
```

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `middle_square.h` | Cabecera con tipos y declaraciones |
| `middle_square.c` | Implementación del paso y las medidas de ciclo |
| `middle_square_test.c` | Pruebas, incluida la comparación con `msws64` |
| `README.md` | Este archivo |

## Uso

```c
#include "middle_square.h"

middle_square_t rng;
middle_square_semilla(&rng, 1234);
uint32_t x = middle_square_siguiente(&rng);        /* 5227 */
double d  = middle_square_siguiente_double(&rng);   /* x / 10000 en [0,1) */
uint32_t k = middle_square_siguiente_acotado(&rng, 7);
```

Funciones de diagnóstico:

| Función | Qué mide |
|---------|----------|
| `middle_square_periodo(rng, max_pasos)` | pasos hasta volver al estado actual (0 si el estado es transitorio) |
| `middle_square_periodo_ciclo(semilla, max_pasos)` | longitud del ciclo eventual tras el transitorio |
| `middle_square_secuencia(semilla, destino, capacidad)` | la órbita completa hasta que se repite |

## Qué muestra este generador (medido, no supuesto)

El módulo no disimula las limitaciones del método: las **mide** y las deja
documentadas en las pruebas.

| Magnitud | Valor medido |
|----------|--------------|
| Ciclo más largo del generador | **4** estados: 6100 → 2100 → 4100 → 8100 → 6100 |
| Semillas que caen en el sumidero 0 | **1968 de 10000** (19,7 %) |
| Otros puntos fijos | 100, 2500 |
| Órbita de la semilla 1234 | 63 estados hasta caer en 0 |
| χ² del mapa `x → x'` (10 bins, 10000 pasos) | **74,18** (valor crítico al 99 %: 21,67) |
| χ² de `msws64` (10 bins, 100000 muestras) | **9,78** |

Es decir: el *middle square* de 4 dígitos **no es un buen generador**. Sus
ciclos son de apenas unos pocos estados, una de cada cinco semillas se
estanca en cero y el sesgo de los cuadrados es detectable (χ² = 74 frente a
9,8 del `msws64` de 64 bits). Por eso la comparación con `msws64` no es
decorativa: el módulo cuantifica la diferencia.

## Comparación con `msws64`

- `msws64` (`../msws64/msws64.h`) es un multiplicador congruencial lineal de
  64 bits con xorshift: periodo del orden de 2⁶⁴ y distribución uniforme
  (χ² = 9,78 con 100000 muestras).
- `msws64.c` contiene además una copia **estática** (no exportada) de este
  generador, `ms_next_4`, con exactamente el mismo criterio de dígitos
  centrales. El test la replica y verifica que las dos implementaciones
  producen la misma secuencia, estado por estado.

## Compilar y probar

```bash
gcc -std=c11 -O2 -Wall -Wextra middle_square.c middle_square_test.c -lm -o prueba
./prueba
```

Compila limpio con `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Werror`.

## Referencias

- J. von Neumann, "Various techniques used in connection with random digits",
  *National Bureau of Standards Applied Mathematics Series* 12 (1946): método
  del *middle square*.
- J. W. Tukey, "The dangers of random digit generators", *Mathematical
  Tables* 3 (1960): análisis de la periodicidad y del sesgo de los cuadrados.
