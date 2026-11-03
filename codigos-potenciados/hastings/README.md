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

| Función | Rango | Grado | Error aproximado (Hastings) |
|---------|-------|-------|-----------------------------|
| `sin(x)` | [0, π/2] | 5 | ~1e-5 |
| `atan(x)` | [0, 1] | 5 | ~1.0e-5 |
| `exp(x)` | [0, 1] | 5 | ~1.0e-5 |
| `log(1+x)` | [0, 1] | 5 | ~1.0e-5 |

## Cómo compilar y ejecutar

```bash
cd codigos-potenciados/hastings
make
./hastings_test
```

Salida esperada (ejemplo):

```
=================================================================
  PRUEBAS DE PRECISIÓN - Hastings (1955) vs Remez
=================================================================

=== sin(x) en [0, pi/2] ===
  Hastings original:    x con error max: 1.5707963268  error: 2.3e-06
  Remez refinado:       x con error max: 1.5707963268  error: 3.1e-08
    mejora:    74.0x

=== atan(x) en [0, 1] ===
  Hastings original:    x con error max: 1.0000000000  error: 1.2e-06
  Remez refinado:       x con error max: 0.9999999999  error: 2.1e-08
    mejora:    57.0x

=== exp(x) en [0, 1] ===
  Hastings original:    x con error max: 1.0000000000  error: 8.4e-07
  Remez refinado:       x con error max: 0.9999999999  error: 1.9e-09
    mejora:    442.0x

=== log(1+x) en [0, 1] ===
  Hastings original:    x con error max: 1.0000000000  error: 1.2e-06
  Remez refinado:       x con error max: 1.0000000000  error: 1.9e-08
    mejora:    63.0x

=================================================================
  FIN DE PRUEBAS
=================================================================
```

## Cómo funciona el refinamiento de Remez

El algoritmo de intercambio de Remez encuentra el polinomio de grado n
que minimiza el error absoluto máximo en un intervalo [a, b]:

1. **Selecciona** puntos iniciales equiespaciados en el intervalo.
2. **Resuelve** el sistema lineal para encontrar coeficientes c_j y error E
   tal que: f(x_i) - P(x_i) = (-1)^i * E en los puntos seleccionados.
3. **Encuentra** los puntos de error extremal (máximos y mínimos locales).
4. **Reemplaza** los puntos anteriores con los nuevos extremos.
5. **Itera** hasta que el error cambie menos que la tolerancia.

El resultado es un polinomio de minimax: el mejor aproximador polinomial
de grado n en el sentido del máximo error absoluto.

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
rp.degree = 5;
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