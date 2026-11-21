# evolve-agent / codigos-potenciados

Repositorio de trabajo del agente `evolve-agent`. Contiene código C bajo `codigos-potenciados/` con implementaciones de algoritmos numéricos y PRNGs, listos para uso embebido (Arduino/AVR compatible).

> ## Aviso (léase primero)
>
> - **`randu_plus` NO es una mejora de RANDU.** Es una *demostración didáctica*
>   de por qué no lo es: `fmix32` es una biyección, así que la salida determina
>   el estado y el retículo de planos (15 planos, medido) sigue siendo
>   recuperable. Su período real es 2³⁰. Se conserva como ejemplo histórico.
> - **Lo que sí mejoró RANDU está en otros módulos**, y esos son los que deben
>   usarse: `codigos-potenciados/mult_xorshift64/` (generador de 64 bits mult-xorshift), `hybrid_prng/`
>   (middle-square + generador de 64 bits) y el **híbrido LCG+mult_xorshift64** de
>   `lcg_lehmer/`.
> - **Corrección (2026-11-20):** el módulo se llamaba `msws64` y se atribuía a
>   Widynski (2017). **El nombre y la atribución eran falsos.** El generador es
>   una cadena de multiplicación-xorshift de 3 rondas y ahora se llama
>   `codigos-potenciados/mult_xorshift64/`. El msws64 real de Widynski (cuadrado + incremento
>   de Weyl + rotación de 32 bits) está implementado en
>   `codigos-potenciados/msws_widynski/` (verificado bit a bit contra la
>   implementación de referencia del autor; el crate Rust `msws` cubre solo
>   `msws32` y el crate Rust `weyl-rs` solo la variante mono); el xorshift64* de
>   Vigna es un tercer generador distinto.
>   Los tres dan salidas diferentes desde la primera iteración.
> - **Los informes de este repo son estudios, no homologaciones.** Nada de lo
>   que hay aquí certifica un estándar (ISO/IEC 23837, PractRand) ni una
>   calidad criptográfica. Lo que sí hacen es *medir* con p-values publicables.

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
| `hastings/` | Aproximaciones polinómicas de sin, atan, exp y log1p con los coeficientes de Hastings (1955) + reducción de argumento; log, asin, acos, atan2, pow y sqrt se derivan de ellos; Remez minimax opcional (grado 11 en x para sin/atan, 5 para exp, 9 para log1p) | Listo para Arduino, error 1e-5 o mejor; Remez minimax opcional |
| `cordic/` | CORDIC (Volder 1959): seno/coseno solo con sumas, restas y shifts | Listo para Arduino |
| `lcg_lehmer/` | Análisis espectral de LCG (RANDU, MINSTD), prueba de período, híbrido LCG+mult_xorshift64, TCR-LCG | Completo |
| `mult_xorshift64/` | Generador de 64 bits (multiplicación + xorshift de 3 rondas) + middle-square de von Neumann (referencia) | Completo |
| `msws_widynski/` | MSWS de Widynski (2017): cuadrado + Weyl + rotación, en variantes de 32 bits, 64 bits de dos carriles y 64 bits mono; verificado bit a bit contra la implementación de referencia del autor, el crate Rust `msws` (solo `msws32`) y el crate Rust `weyl-rs` (solo la variante mono) | Completo (32/32 tests) |
| `middle_square/` | Generador de von Neumann de 4 dígitos con ciclo, sumideros y comparación contra `mult_xorshift64` | Completo (17/17 tests) |
| `hybrid_prng/` | PRNG híbrido middle-square + generador de 64 bits, con tests estadísticos y unitarios | Completo |
| `abacus_vedic/` | Suanpan chino sobre cuentas (suma, resta, multiplicación) + multiplicación védica (urdhva) y cruces | Completo (29/29 tests) |
| `pi_ancestral/` | π por polígonos de Arquímedes (cotas inscrita/circunscrita) + extrapolación de Liu Hui con Richardson | Completo (30/30 tests) |
| `randu_plus.c`, `sine_fixed.c` | Utilidades sueltas | Completo (ver nota abajo) |

> **Nota sobre `randu_plus`.** RANDU + fmix32 **no** es un buen generador: fmix32
> es una biyección, así que la salida determina el estado y el retículo de RANDU
> sigue siendo recuperable; el período real es 2³⁰. Se conserva como ejemplo
> histórico con la advertencia explícita, y el propio test lo verifica (8/8).
> Para generar números usa `codigos-potenciados/mult_xorshift64/`.
>
> **Ojo con el nombre:** el RANDU histórico de Knuth usa módulo 2³¹ y período
> 2²⁹; este repo usa una **variante con módulo 2³²** y período **2³⁰** (medido,
> ver `randu_plus.c`). Comparten multiplicador (65539) y defecto de planos, pero
> no son el mismo generador.

## Generadores que sí son utilizables (lo que mejoró RANDU)

Si lo que interesa es producir números pseudoaleatorios de calidad, estos son
los módulos del repo, con su ruta y su test:

| Módulo | Rutas exactas | Qué aporta frente a un LCG lineal |
|---|---|---|
| `mult_xorshift64` | `codigos-potenciados/mult_xorshift64/mult_xorshift64.h`, `mult_xorshift64.c`, `README.md` | PRNG de 64 bits (mult-xorshift de 3 rondas); **no** es el msws64 de Widynski ni el xorshift64* de Vigna; su período no está verificado y el sumidero 0 se evita forzando semilla impar |
| `msws_widynski` | `codigos-potenciados/msws_widynski/msws_widynski.h`, `msws_widynski_test.c`, `README.md` | MSWS de Widynski (2017) en 3 variantes (32 bits, 64 bits de dos carriles, 64 bits mono), con salto adelante propio; verificado bit a bit contra la implementación de referencia del autor (las 3 variantes) y, además, contra el crate Rust `msws` (**solo `msws32`**) y el crate Rust `weyl-rs` (**solo la variante mono**). Para `msws64` de dos carriles no hay oráculo publicado de terceros: la verificación es contra el header del autor y contra una reimplementación independiente escrita desde el artículo |
| `hybrid_prng` | `codigos-potenciados/hybrid_prng/hybrid_prng.h`, `hybrid_prng.c`, `hybrid_prng_test.c` | middle-square + generador de 64 bits: el estado entra por el square, la salida sale por el generador de 64 bits |
| `lcg_lehmer` | `codigos-potenciados/lcg_lehmer/lcg_lehmer.h`, `lcg_lehmer.c`, `lcg_lehmer_test.c` | análisis espectral de LCG (RANDU, MINSTD), prueba de período, **híbrido LCG+mult_xorshift64** y TCR-LCG |
| `middle_square` | `codigos-potenciados/middle_square/` | von Neumann de 4 dígitos con ciclo, sumideros y comparación contra `mult_xorshift64` (17/17 tests) |

Los tres compilan sin warnings con `gcc -std=c11 -O2 -Wall -Wextra -pedantic` y
sus tests pasan. `lcg_lehmer_test` **mide** el defecto de RANDU por dentro
(espectral 3D = 0.0084 → MALO, “solo ~15 planos”) y el de MINSTD (0.4952 →
aceptable), que es la razón por la que el repo no insiste en RANDU.

### Medición de esos tres generadores (batería propia, 2026-11-20)

`prng_modules_validator.py` mide el flujo de 32 bits de `mult_xorshift64`,
`hybrid_prng` y del híbrido LCG+mult_xorshift64 con 8 semillas × 7 tests (56
p-values por generador) y dos controles: `/dev/urandom` (limpio, debe
aprobar) y el estado crudo de `randu_plus` (defectuoso, debe fallar).
Resultado en `VALIDATION_REPORT_PRNG_MODULES.md`:

| Flujo | p < 0.01 | p < 0.001 | Lectura |
|---|---|---|---|
| `/dev/urandom` (control) | 1/56 | 0 | el control limpio también produce algún p<0.01: exigir 56/56 daría falsos FAIL |
| `mult_xorshift64` (mult-xorshift de 3 rondas, **no** el de Widynski) | 0/56 | 0 | sin señal en la batería |
| `hybrid_prng` | 1/56 | 0 | señal marginal reproducible en `NIST_BlockFreq` (semilla 65537, p=0.00757203) |
| LCG+mult_xorshift64 | 0/56 | 0 | sin señal en la batería |

Matices honestos:

- La señal marginal de `hybrid_prng` es **reproducible** (mismo valor en
  corridas independientes) y queda registrada como señal, no como prueba de
  defecto: la parte middle-square aporta solo ~13,3 bits por palabra, así que
  su mitad superior hereda esa limitación aunque el XOR con el generador de 64
  bits la disperse.
- El FAIL de `NIST_Runs` que se vio antes sobre `/dev/urandom` y el de
  `MarsagliaSpectral3D` (p=0.00043) sobre LCG+mult_xorshift64 eran **ruido del test**:
  el segundo desaparece al pasar de 50 000 a 200 000 tuplas sobre el mismo
  flujo (p=0.0751181). Por eso el test espectral usa 200 000 tuplas.
- Esto **no es** TestU01 ni PractRand: TestU01 real no está instalado en este
  entorno y el harness versionado solo cubre `randu_plus`.

Recomendación: para uso general, el generador de 64 bits del módulo `mult_xorshift64/`
(es el único con 0/56 en toda la batería). `hybrid_prng` y el híbrido
LCG+mult_xorshift64 pasan, pero con las salvedades anteriores.

> **Qué se midió exactamente:** estas cifras corresponden al generador que el
> módulo `mult_xorshift64/` implementa (cadena de multiplicación-xorshift de 3
> rondas), **no** al msws64 de Widynski. El msws64 de Widynski
> (`codigos-potenciados/msws_widynski/`) tiene su propia batería de verificación
> contra fuentes externas, pero **todavía no está sometida a esta batería
> estadística** de p-valores.

Lo que **no** hay todavía (y conviene no dar por hecho): un harness de TestU01
o una corrida de PractRand para `mult_xorshift64`/`hybrid_prng`. El único harness de
TestU01 del repo es el de `randu_plus` (`codigos-potenciados/testu01/`).

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
mide el flujo de bits con una batería de tests estándar (espectro 3D de
Marsaglia con p-value, espaciado de cumpleaños, permutaciones, frecuencia
monobit, frecuencia por bloques y rachas):

```bash
python3 randu_plus_validator.py        # regenera VALIDATION_REPORT.md
```

El barrido usa **8 semillas impares distintas** y el criterio de paso es el
**peor caso entre semillas** (el mínimo de los p-values), no el de una
ejecución afortunada. El informe (`VALIDATION_REPORT.md`) documenta el consumo
real de muestras de cada test y sus límites: los tests de primer orden **no**
detectan el retículo de RANDU por sí solos. Para eso están la batería de
TestU01 (SmallCrush), que sí lo detecta cuando recibe el estado crudo, y el
**control negativo** `LatticeNegativeControl`, que aplica el test espectral de
Marsaglia al estado crudo (p-value 0, retículo de **15 planos**) y a la salida
mezclada (p-value 0.919), y comprueba que `unmix32(salida)` devuelve el estado
crudo terna a terna. El test incluido en `randu_plus.c` verifica además
biyectividad, período real y recuperación del retículo.

El harness de TestU01 está en `codigos-potenciados/testu01/` (fuentes y
`Makefile`; los binarios no se versionan). Requiere TestU01 instalado:

```bash
make -C codigos-potenciados/testu01 TESTU01_HOME=/ruta/a/testu01
./codigos-potenciados/testu01/run_randu_plus smallcrush state 12345   # debe fallar
./codigos-potenciados/testu01/run_randu_plus smallcrush fmix  12345   # pasa
```

`lattice_recovery` (`make -C codigos-potenciados/testu01 check`) demuestra por
qué: el retículo se **recupera bit a bit** desde la salida de `randu_plus()`,
porque `fmix32` es invertible.

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

# msws_widynski (Widynski 2017: 32 bits, 64 bits de dos carriles, 64 bits mono)
gcc -std=c11 -O2 -Wall -Wextra -pedantic codigos-potenciados/msws_widynski/msws_widynski_test.c -o /tmp/msws_widynski_test
/tmp/msws_widynski_test

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