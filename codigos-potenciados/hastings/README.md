# hastings - Polinomios de Hastings (1955) con refinamiento por Remez

Implementa aproximaciones polinómicas de funciones trascendentales basadas
en las tablas de Hastings et al. (1955), con refinamiento opcional mediante
el algoritmo de intercambio de Remez para minimizar el error máximo.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `hastings.h` | Declaraciones de funciones y estructuras |
| `hastings.c` | Implementación de las formulaciones originales + algoritmo de Remez |
| `hastings_test.c` | Pruebas de precisión comparando contra libm |
| `Makefile` | Compilación |
| `README.md` | Este archivo |

## Formulaciones incluidas

| Función | Rango | Polinomio | Error absoluto medido |
|---------|-------|-----------|----------------------|
| `sin(x)` | [0, 2π] (reducido a [0, π/2]) | x·P₅(x²) | 5.6e-08 |
| `atan(x)` | [0.05, 4] | x·P₅(x²) | 7.7e-06 |
| `exp(x)` | [-5, 5] | 1 + x·P₅(x) | 8.3e-06 relativo |
| `log1p(x)` | [0, 1] | x·P₉(x) | 2.9e-06 |

Las reducciones de argumento son:

- `sin`: se reduce a [0, π/2] con `sin(2π − x) = −sin(x)` y `sin(π − x) = sin(x)`,
  conservando el signo en todo el rango.
- `atan`: `atan(x) = π/4 + atan((x−1)/(x+1))` para x ∈ [0.5, 2] y
  `atan(x) = π/2 − atan(1/x)` para x > 2.
- `exp`: `x = k·ln2 + r` con 0 ≤ r < ln2, y `exp(x) = 2ᵏ·p(r)`; para x < 0 se
  usa `1/exp(−x)`; para 0 ≤ x ≤ 1 se reduce x a ≤ 0.25 y se eleva al cuadrado.
- `log1p`: `log1p(x) = ln2 + log1p((x−1)/2)` para x ≥ 0.25.

## Cómo compilar y ejecutar

```bash
cd codigos-potenciados/hastings
make
./hastings_test
```

El test imprime el detalle de cada comprobación y termina con un resumen
`N/N pruebas pasaron`; devuelve código de salida 1 si alguna falla.

Salida esperada (ejemplo):

```
=================================================================
  PRUEBAS DE PRECISIÓN - Hastings (1955) vs Remez
=================================================================

=== sin(x) ===
  [OK] sin error max en [0,pi/2] <= 1e-6  error max en [0,pi/2] = 5.626e-08 (x=1.570796)
  [OK] sin error max en [0,2pi] <= 1e-6  error max en [0,2pi] = 5.626e-08 (x=1.570796)
  [OK] sin error max en [-2pi,0] <= 1e-6  error max en [-2pi,0] = 5.626e-08
  [OK] sin en puntos clave (incluye 2pi/3 y 3pi/2) <= 1e-6  error max = 5.626e-08
  [OK] sin es impar en [0,2pi] <= 1e-6  |sin(-x)+sin(x)| max = 0.000e+00

=== Remez (minimax) ===
  [OK] Remez(grado 5) mejora a Taylor(grado 5)  sin: Remez(5) = 7.071e-06, Taylor(5) = 4.525e-03
  [OK] Remez(grado equivalente) no es peor que Hastings  sin: Remez(11) = 3.994e-14, Hastings = 5.626e-08

=================================================================
  33/33 pruebas pasaron
=================================================================
```

## Cómo funciona el refinamiento de Remez

El algoritmo de intercambio de Remez encuentra el polinomio de grado n
que minimiza el error absoluto máximo en un intervalo [a, b]:

1. Parte de m = n+2 puntos equiespaciados en el intervalo.
2. Resuelve el sistema lineal para obtener los coeficientes c_j y el error E
   tal que: `f(x_i) − P(x_i) = (−1)^i · E` en los puntos seleccionados.
3. Muestrea el error y localiza sus extremos locales en valor absoluto,
   junto con los extremos del intervalo.
4. Selecciona m puntos con **signos alternados y en orden creciente**: el
   intercambio exige alternancia en ese orden, no solo signos alternados.
5. Itera hasta que `|E_nuevo − E_anterior| < tolerancia · |E|`.

El resultado es un polinomio de minimax: el mejor aproximador polinomial
de grado n en el sentido del máximo error absoluto.

### Ojo con los grados

El polinomio de Hastings para `sin` y `atan` tiene la forma `x·P(x²)` con
P de grado 5, es decir **grado 11 en x**. Para `exp` es grado 6 y para
`log1p` grado 10. Un Remez de grado 5 no puede mejorar a esos polinomios
(para `sin` en [0, π/2] el minimax de grado 5 da 7.1e-06 frente a los
5.6e-08 de Hastings): la comparación justa usa el grado equivalente.

Grados equivalentes: 11 para `sin` y `atan`, 6 para `exp`, 10 para `log1p`.

| Función | Hastings | Remez (grado equivalente) | Mejora |
|---------|----------|---------------------------|--------|
| `sin` [0, π/2] | 5.63e-08 | 3.99e-14 | 1.4e6x |
| `atan` [0, 1] | 7.63e-06 | 1.71e-09 | 4.5e3x |
| `exp` [0, 1] | 3.89e-08 | 4.03e-08 | 0.97x |
| `log1p` [0, 1] | 2.86e-06 | 7.09e-10 | 4.0e3x |

Para `exp` el Remez queda apenas por detrás: el sistema en base monomial con
puntos de Vandermonde pierde precisión numérica en ese intervalo, y el
error medido ronda 1e-08.

## Uso directo

```c
#include "hastings.h"

// Formulaciones originales de Hastings (1955)
double y = hastings_sin(0.5).value;
double y = hastings_atan(0.5).value;
double y = hastings_exp(0.5).value;
double y = hastings_log1p(0.5).value;

// Refinamiento con Remez
remez_params_t rp = {0};
rp.f = sin;
rp.a = 0.0;
rp.b = M_PI/2.0;
rp.degree = 11;         // grado equivalente al polinomio de Hastings
rp.max_iterations = 100;
rp.tolerance = 1e-14;
remez_solve(&rp);
double y = remez_evaluate(&rp, 0.5);  // valor aproximado
double err = remez_max_error(&rp, 1000);  /* error máximo en el rango */
remez_free(&rp);
```

## Referencias

- Hastings, C. et al. (1955). *Approximations for Digital Computers*. Princeton University Press.
- Remez, E. Ya. (1934). "Sur la détermination des polynômes d'approximation de degré donné". Comm. Soc. Math. Kharkov.
- Knuth, D. E. (1981). *The Art of Computer Programming, Vol. 2*, Section 4.5.
