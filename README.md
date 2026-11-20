# evolve-agent / codigos-potenciados

Repositorio de trabajo del agente `evolve-agent`. Contiene código C bajo `codigos-potenciados/` con implementaciones de algoritmos numéricos y PRNGs, listos para uso embebido (Arduino/AVR compatible).

## Estructura

| Path | Contenido |
|------|-----------|
| `AGENTS.md` | Reglas del agente: identidad, misión, principios, bucle de auto-evolución. |
| `KNOWLEDGE.md` | Índice de conocimiento actual. |
| `codigos-potenciados/` | Implementaciones C (ver tabla abajo). |
| `notes/` | Logs diarios fechados. No se editan entradas pasadas. |

## codigos-potenciados/

| Módulo | Descripción | Estado |
|--------|-------------|--------|
| `goertzel/` | Algoritmo Goertzel con 3 formulaciones (K, α, β) + híbrida de 3 zonas con plegado por aliasing + FFT radix-2 de referencia | Listo para Arduino (float), DTMF |
| `hastings/` | Aproximaciones polinómicas de sin, atan, atan2, asin, sqrt, exp, log1p, log, pow (grado 5 en x², hasta grado 11 en x) | Listo para Arduino, error 1e-5 o mejor; Remez minimax opcional |
| `cordic/` | CORDIC (Volder 1959): seno/coseno solo con sumas, restas y shifts | Listo para Arduino |
| `lcg_lehmer/` | Análisis espectral de LCG (RANDU, MINSTD), prueba de período, híbrido LCG+msws64, TCR-LCG | Completo |
| `msws64/` | PRNG msws64 (Widynski 2017) de 64 bits + middle-square de von Neumann (referencia) | Completo |
| `middle_square/` | Generador de von Neumann de 4 dígitos con ciclo, sumideros y comparación contra `msws64` | Completo (17/17 tests) |
| `hybrid_prng/` | PRNG híbrido middle-square + msws64 con tests estadísticos y unitarios | Completo |
| `abacus_vedic/` | Suanpan chino sobre cuentas (suma, resta, multiplicación) + multiplicación védica (urdhva) y cruces | Completo (29/29 tests) |
| `pi_ancestral/` | π por polígonos de Arquímedes (cotas inscrita/circunscrita) + extrapolación de Liu Hui con Richardson | Completo (30/30 tests) |
| `randu_plus.c`, `sine_fixed.c` | Utilidades sueltas | Completo (ver nota abajo) |

> **Nota sobre `randu_plus`.** RANDU + fmix32 **no** es un buen generador: fmix32
> es una biyección, así que la salida determina el estado y el retículo de RANDU
> sigue siendo recuperable; el período real es 2³⁰. Se conserva como ejemplo
> histórico con la advertencia explícita, y el propio test lo verifica (8/8).
> Para generar números usa `codigos-potenciados/msws64/`.

## `randu_plus` (ejemplo histórico, no usar para números pseudoaleatorios)

API (header único, sin dependencias, compatible con AVR/Arduino):

```c
#include "randu_plus.h"

uint32_t state = 12345u;          /* semilla: se fuerza impar (periodo 2^30) */
uint32_t r = randu_plus(&state);  /* siguiente valor de 32 bits */
```

| Función / macro | Para qué sirve |
|-----------------|----------------|
| `randu_plus(uint32_t *state)` | Genera el siguiente `uint32_t`. El estado se fuerza impar con `*state \|= 1u`. |
| `randu_plus_unfmix32(uint32_t y)` | Inversa de `fmix32`: recupera el estado interno desde la salida. |
| `randu_plus_inverse_multiplier(void)` | Inversa multiplicativa de 65539 mod 2³², para retroceder en el LCG. |
| `RANDU_PLUS_PERIOD` | Período real medido del LCG con semilla impar: `1073741824u` (2³⁰). |

El header está duplicado en dos rutas (raíz y `codigos-potenciados/`); ambas
copias deben quedar **byte a byte idénticas** y el validador lo comprueba.

### Validación estadística

`randu_plus_validator.py` compila un driver contra el header real del repo y
mide el flujo de bits con una batería de tests estándar (espectro 3D,
espaciado de cumpleaños, permutaciones, frecuencia monobit, frecuencia por
bloques y rachas):

```bash
python3 randu_plus_validator.py        # regenera VALIDATION_REPORT.md
```

El barrido usa **8 semillas impares distintas** y el criterio de paso es el
**peor caso entre semillas** (el mínimo de los p-values), no el de una
ejecución afortunada. El informe (`VALIDATION_REPORT.md`) documenta el consumo
real de muestras de cada test y sus límites: los tests de primer orden **no**
detectan el retículo de RANDU. Para eso está la batería de TestU01
(SmallCrush), que sí lo detecta cuando recibe el estado crudo, y el test
incluido en `randu_plus.c`, que verifica biyectividad, período real y
recuperación del retículo.

## Compilar y probar

Cada módulo es independiente. Ejemplo:

```bash
# Goertzel
gcc -std=c11 -O2 -Wall -Wextra codigos-potenciados/goertzel/goertzel.c codigos-potenciados/goertzel/goertzel_test.c -lm -o /tmp/goertzel_test
/tmp/goertzel_test

# Hastings
gcc -std=c11 -O2 -Wall -Wextra codigos-potenciados/hastings/hastings.c codigos-potenciados/hastings/hastings_test.c -lm -o /tmp/hastings_test
/tmp/hastings_test

# revivir (solo test con -DTEST)
gcc -std=c11 -O2 -Wall -Wextra -DTEST revivir.c -lm -o /tmp/revivir_test
/tmp/revivir_test

# hybrid_prng (stats y unit tests por separado)
gcc -std=c11 -O2 -Wall -Wextra -DHYBRID_PRNG_MAIN codigos-potenciados/hybrid_prng/hybrid_prng.c -lm -o /tmp/hybrid_prng_stat
gcc -std=c11 -O2 -Wall -Wextra codigos-potenciados/hybrid_prng/hybrid_prng_test.c codigos-potenciados/hybrid_prng/hybrid_prng.c -lm -o /tmp/hybrid_prng_test

# middle_square (con middle_square_test.c)
gcc -std=c11 -O2 -Wall -Wextra codigos-potenciados/middle_square/middle_square.c codigos-potenciados/middle_square/middle_square_test.c -lm -o /tmp/middle_square_test

# abacus_vedic
gcc -std=c11 -O2 -Wall -Wextra codigos-potenciados/abacus_vedic/abacus_vedic.c codigos-potenciados/abacus_vedic/abacus_vedic_test.c -lm -o /tmp/abacus_vedic_test

# pi_ancestral
gcc -std=c11 -O2 -Wall -Wextra codigos-potenciados/pi_ancestral/pi_ancestral.c codigos-potenciados/pi_ancestral/pi_ancestral_test.c -lm -o /tmp/pi_ancestral_test

# sine_fixed (raíz): Q16.16 con tabla e interpolación, test con criterio
cc -std=c99 -O2 -Wall -Wextra -pedantic -Werror -DTEST sine_fixed.c -lm -o /tmp/sine_fixed_test
/tmp/sine_fixed_test

# sine_fixed (codigos-potenciados): Q16.16 y Q32.32 con índice entero
cc -std=c99 -O2 -Wall -Wextra -pedantic -Werror -DTEST codigos-potenciados/sine_fixed.c -lm -o /tmp/sine_cp_test
/tmp/sine_cp_test

# randu_plus: verifica biyectividad, retículo recuperable y período real
cc -std=c99 -O2 -Wall -Wextra -pedantic -Werror randu_plus.c -o /tmp/randu_test
/tmp/randu_test
```

Los módulos nuevos (`goertzel`, `middle_square`, `abacus_vedic`, `pi_ancestral`)
compilan con **0 warnings** bajo
`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Werror`.
Para regenerar la base de datos de análisis estático:

```bash
python3 codigos-potenciados/tools/gen_compile_commands.py
```

Todos los módulos compilan con `0 warnings` bajo `-Wall -Wextra`. No hay binarios commiteados (`.gitignore` cubre `*_test`, `goertzel_bench`, `sine_fixed`, `*.o`).

## Startup Order

1. Leer `AGENTS.md`.
2. Leer `KNOWLEDGE.md`.
3. Leer las últimas 2-3 entradas en `notes/`.
4. Revisar el goal, worktree, código, datos y logs.

## Reglas de memoria

- `knowledge/` almacena solo hechos actuales; actualizar cuando cambien.
- `notes/` almacena logs diarios; añadir durante el día, no editar entradas pasadas.
- Cambios de principio van directo en `AGENTS.md`.