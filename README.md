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
| `goertzel/` | Algoritmo Goertzel con 3 formulaciones (K, α, β, híbrida) + FFT radix-2 de referencia | Listo para Arduino (float), DTMF |
| `hastings/` | Aproximaciones polinómicas de sin, atan, atan2, asin, sqrt, exp, log1p, log, pow (grado 5-8) | Listo para Arduino, error 1e-5 o mejor |
| `cordic/` | CORDIC (Volder 1959): seno/coseno solo con sumas, restas y shifts | Listo para Arduino |
| `lcg_lehmer/` | Análisis espectral de LCG (RANDU, MINSTD), prueba de período, híbrido LCG+msws64, TCR-LCG | Completo |
| `msws64/` | PRNG msws64 (Widynski 2017) de 64 bits + middle-square de von Neumann (referencia) | Completo |
| `middle_square/` | Generador de von Neumann de 4 dígitos con ciclo, sumideros y comparación contra `msws64` | Completo (17/17 tests) |
| `hybrid_prng/` | PRNG híbrido middle-square + msws64 con tests estadísticos y unitarios | Completo |
| `abacus_vedic/` | Suanpan chino sobre cuentas (suma, resta, multiplicación) + multiplicación védica (urdhva) y cruces | Completo (29/29 tests) |
| `pi_ancestral/` | π por polígonos de Arquímedes (cotas inscrita/circunscrita) + extrapolación de Liu Hui con Richardson | Completo (30/30 tests) |
| `randu_plus.c`, `sine_fixed.c` | Utilidades sueltas | Completo |

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