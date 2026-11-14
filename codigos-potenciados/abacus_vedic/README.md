# abacus_vedic - Suanpan (abaco chino) y multiplicación védica (urdhva)

Dos algoritmos ancestrales implementados **sobre las cuentas**, no sobre
aritmética moderna de enteros: el **suanpan** chino y la **multiplicación
védica** (urdhva: producto parcial por cada dígito del multiplicador,
desplazado en potencias de diez y sumado).

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `abacus_vedic.h` | Cabecera con tipos y declaraciones |
| `abacus_vedic.c` | Implementación (cuentas, acarreo, préstamo, urdhva) |
| `abacus_vedic_test.c` | Pruebas numéricas y de formato |
| `README.md` | Este archivo |

## Uso

```c
#include "abacus_vedic.h"

suanpan_estado_t a, b, r;
suanpan_desde_decimal(&a, 1234);
suanpan_desde_decimal(&b, 5678);

suanpan_suma(&r, &a, &b);            /* 6912, con acarreo de poste en poste */
suanpan_resta(&r, &r, &b);           /* 1234, con préstamo de poste en poste */
suanpan_multiplica_digito(&r, &a, 7);/* 8638 */
vedic_multiplicar(&r, &a, &b);       /* 7006652, sin multiplicación "larga" */
```

## El ábaco

Cada poste tiene dos cuentas de **cielo** (valor 5) y cinco de **tierra**
(valor 1). Para el cálculo se usa la forma canónica sin redundancia:

```
dígito del poste = 5·cielo + tierra        cielo ∈ {0,1}, tierra ∈ 0..4
```

Es la misma convención que usan las herramientas Octave `suanpan_add` y
`vedic_multiply` del ecosistema MCP, de modo que los resultados se pueden
comparar dígito a dígito:

| decimal | cielo (MSB→LSB) | tierra (MSB→LSB) |
|---------|------------------|-------------------|
| 4832    | 0 1 0 0          | 4 3 3 2           |
| 7       | 1                | 2                 |

El índice 0 de `suanpan_estado_t` es el poste de las **unidades** (orden en
que viajan los acarreos); `suanpan_a_cadenas()` / `suanpan_desde_cadenas()`
convierten al formato de las herramientas (índice 0 = dígito más
significativo).

> El suanpan tradicional (2 de cielo + 5 de tierra) permite representaciones
> redundantes hasta 15 por poste; este módulo normaliza a 0..9, que es lo que
> hacen las herramientas de referencia para poder calcular.

## Multiplicación védica (urdhva)

En vez de la multiplicación "larga" (que multiplica cada dígito por todos los
dígitos del otro número), el método védico:

1. multiplica el multiplicando por **cada dígito** del multiplicador
   (`suanpan_multiplica_digito`),
2. desplaza cada producto parcial en potencias de diez,
3. suma los parciales sobre el abaco (`suanpan_suma`).

`vedic_cruces()` expone los **productos cruzados** dígito a dígito
(convolución de las columnas), que es la cantidad que la herramienta Octave
reporta como `cross_products`.

## Compilar y probar

```bash
gcc -std=c11 -O2 -Wall -Wextra abacus_vedic.c abacus_vedic_test.c -lm -o prueba
./prueba
```

Compila limpio con `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Werror`.

## Referencias

- *Suànpan Shùyàn* (算盤術, "El origen del ábano"), 1594: manual oficial del
  ábaco chino.
- A. P. Śāstrī, *Āryabhatiya* (499 d.C.), cap. 2 ("khanda 'prayoga",
  productos parciales por cifras) y *Ganita-sāra*, cap. 8: el método urdhva
  (vertical) de multiplicación.
