# Análisis estático de `codigos-potenciados` vía VS Code MCP

Fecha: 2026-11-08
Workspace VS Code: `/home/astrid/Documents/OpenScience/projects/githu`
Bridge: `yutengjing.vscode-mcp-bridge` v4.9.5 + `@vscode-mcp/vscode-mcp-server` v4.9.5
Language server: `ms-vscode.cpptools` v1.35.6 (instalado en esta sesión)

## 1. Alcance

25 archivos C/H de los 7 módulos (`cordic`, `goertzel`, `hastings`, `hybrid_prng`,
`lcg_lehmer`, `msws64`, `matematica_ancestral`) más 2 archivos sueltos en la raíz
(`randu_plus.c`, `sine_fixed.c`).

Los sketches `.ino` (`dtmf_gen.ino`, `dtmf_decoder.ino`) quedan **fuera** del análisis
C/C++ de cpptools (extensión no reconocida por el language server).

## 2. Estado del puente VS MCP (estaba roto)

Síntoma: `list_workspaces` devolvía 0 workspaces y ninguna herramienta MCP funcionaba.

Causa raíz: la extensión (dentro de VS Code) y el servidor MCP calculan el path del
socket Unix a partir de `XDG_DATA_HOME`, pero corrían con valores distintos:

| Lado | `XDG_DATA_HOME` | Path del socket | Longitud |
|------|-----------------|-----------------|----------|
| Extensión (VS Code heredaba el entorno del workbench) | `/home/astrid/.local/share/com.ai4s.workbench/runtime/xdg-data` | `.../yutengjing-vscode-mcp/vscode-mcp-ce37c3b8.sock` | **111** |
| Servidor MCP (`~/.config/opencode/opencode.json` → `environment`) | `/home/astrid/.local/share` | `/home/astrid/.local/share/yutengjing-vscode-mcp/vscode-mcp-ce37c3b8.sock` | **68** |

El límite de `sun_path` en AF_UNIX es 107 bytes útiles (108 con NUL). Con 111 caracteres
el `bind()`/`connect()` falla con `AF_UNIX path too long`, y el servidor terminaba
reportando el socket como "zombi" y limpiándolo.

Arreglo: lanzar VS Code con el mismo `XDG_DATA_HOME` que usa el servidor MCP:

```bash
XDG_DATA_HOME=/home/astrid/.local/share code --new-window \
  --user-data-dir=/home/astrid/.local/share/com.ai4s.workbench/runtime/xdg-config/Code \
  /home/astrid/Documents/OpenScience/projects/githu
```

Verificado: `list_workspaces` → 1 workspace activo; `health_check` → `status: ok`.

## 3. Configuración añadida al repositorio

| Archivo | Propósito |
|---------|-----------|
| `.vscode/c_cpp_properties.json` | IntelliSense C99, `compilerPath=/usr/bin/gcc`, `includePath` por módulo, `-Wall -Wextra -Wpedantic` |
| `.vscode/settings.json` | `C_Cpp.default.compileCommands`, estándar C99/C++17, `*.ino` → `cpp` |
| `codigos-potenciados/tools/gen_compile_commands.py` | Genera `compile_commands.json` (15 entradas) con los flags estrictos |
| `.gitignore` | `/compile_commands.json` ignorado por ser artefacto generado |

El archivo generado se regenera con:

```bash
python3 codigos-potenciados/tools/gen_compile_commands.py
```

## 4. Diagnósticos obtenidos por MCP

`get_diagnostics` (VS Code → C/C++) sobre los 25 archivos, **estado inicial**:
**8 hints, 0 errores, 0 warnings**.

| Archivo | Línea | Include señalado | Verificación empírica |
|---------|-------|------------------|-----------------------|
| `cordic/cordic.c` | 11 | `<math.h>` | **Falso positivo** — `cordic.c` usa `ldexp` (línea 62); sin `<math.h>` falla con *implicit declaration*. cpptools lo considera "no usado" porque trata `ldexp` como builtin |
| `lcg_lehmer/lcg_lehmer.c` | 16 | `"lcg_lehmer.h"` | **Falso positivo** — header requerido (declara `lcg_spectral_result_t`, `lcg_spectral_analyze`, `lcg_period_test`, `lcg_plane_count_3d`) |
| `lcg_lehmer/lcg_lehmer.c` | 17 | `<stdio.h>` | **Removible** — no hay `printf`/`puts` |
| `lcg_lehmer/lcg_lehmer.c` | 18 | `<stdlib.h>` | **Removible** — no hay `malloc`/`rand`/`abs` |
| `msws64/msws64.c` | 16 | `<stdint.h>` | Compila sin él (llega transitivo por `msws64.h`), pero **se mantiene** (IWYU: el TU usa `uint64_t` 12 veces) |
| `msws64/msws64.c` | 17 | `<string.h>` | **Removible** — no hay `memcpy`/`strlen` |
| `matematica_ancestral/matematica_ancestral.c` | 16 | `"matematica_ancestral.h"` | **Falso positivo** — header requerido (declara `fracciones_egipcias`, `sistema_vigesimal_maya`) |
| `matematica_ancestral/matematica_ancestral.c` | 17 | `<stdio.h>` | **Removible** — no hay `printf` |

Método de verificación: se eliminó cada include en una copia temporal y se recompiló con
`gcc -std=c99 -Wall -Wextra -Werror=implicit-function-declaration -fsyntax-only`.

> **Corrección de un error previo.** Una primera versión de esta tabla clasificó
> `<math.h>` en `cordic.c` como removible. El test usado entonces no eliminaba las
> líneas con `<>` (el patrón de borrado sólo cubría la forma `"..."`), de modo que el
> archivo se compilaba *con* el include presente y el resultado era engañoso. Repetido
> con un borrado correcto, `<math.h>` resulta **necesario** por `ldexp`.

### 4.1 Estado tras las correcciones

| Métrica | Antes | Después |
|---------|-------|---------|
| Hints por MCP | 8 | **2** (ambos falsos positivos / intencionales: `cordic.c` `math.h`, `msws64.c` `stdint.h`) |
| Avisos de gcc (flags estrictos) | 28 | **0** |
| `-Werror` con flags estrictos | falla | **pasa en los 15 archivos .c** |
| Pruebas | 7/7 módulos | 7/7 módulos (sin regresiones) |

## 5. Hallazgos de gcc (no visibles en IntelliSense)

En Linux, cpptools no hace *compiler error reporting*: sólo reporta semántica
IntelliSense. Por eso los warnings del compilador se obtienen aparte, con los mismos
flags del `compile_commands.json`:

`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes`

### 5.1 Prototipos faltantes (`-Wmissing-prototypes`)

| Ubicación | Función |
|-----------|---------|
| `cordic/cordic.c:98` | `cordic_radians_to_fixed16` |
| `cordic/cordic.c:105` | `cordic_fixed16_to_radians` |
| `lcg_lehmer/lcg_lehmer.c:56` | `msws64_pure_next` |
| `matematica_ancestral/matematica_ancestral.c:225` | `inverso_modular` |

Las dos de `cordic.c` sí están declaradas en `cordic.h` (el .c no incluye su propio
header); las otras dos no aparecen en ningún header.

### 5.2 Conversiones de signo (`-Wsign-conversion`)

`goertzel/goertzel_test.c:32,63,64,65` · `hastings/hastings.c:349,350,351` ·
`msws64/msws64.c:69,122` — todos `malloc`/`calloc` con multiplicación `int * sizeof`.

### 5.3 Truncamientos (`-Wconversion`)

`hybrid_prng/hybrid_prng.h:41-44` (`uint32_t` → `uint16_t`) ·
`lcg_lehmer/lcg_lehmer.c:65,66,104` (`__uint128_t` → `uint64_t`) ·
`matematica_ancestral/matematica_ancestral.c:105,106` (`int64_t` → `int`).

### 5.4 Calificador `const` descartado (`-Wcast-qual`)

`hastings/hastings.c:274` — `(double *)poly_coeffs[i]` descarta `const`.

### 5.5 cppcheck (estilo/performance)

`cordic/cordic.c:58` (inicialización redundante) ·
`goertzel/goertzel.c:82` (asignación duplicada) · `goertzel/goertzel_test.c:44` (array `const`) ·
`msws64/msws64.c:103` (variable asignada y no usada) ·
`matematica_ancestral/matematica_ancestral.c:196` (scope reducible) ·
`matematica_ancestral/matematica_ancestral_test.c:121,241,263` (arrays `const`).

Ningún hallazgo de severidad `error` ni `warning` en cppcheck (sólo `style`), y estos
8 avisos siguen presentes tras las correcciones: son cosméticos.

## 6. Estado funcional verificado

Todos los módulos compilan y sus pruebas pasan:

| Módulo | Resultado |
|--------|-----------|
| `cordic` | ACEPTABLE — error máx. 3.1e-10 (sin), 1.76e-05 (cos) |
| `goertzel` | Formulaciones K/α/β/híbrida coherentes (16 bits, N=16384) |
| `hastings` | FIN DE PRUEBAS sin fallos |
| `hybrid_prng` | TODOS LOS TESTS PASARON (rango acotado, doble en [0,1)) |
| `lcg_lehmer` | RANDU → MALO en test espectral 3D (esperado); MINSTD aceptable |
| `matematica_ancestral` | 28/28 pruebas aprobadas |
| `msws64` | Secuencia OK (uint64 ↔ double) |

## 7. Limitaciones

- Los `.ino` no reciben diagnósticos C/C++ (cpptools no reconoce la extensión).
- cpptools en Linux no propaga warnings del compilador al panel de Problemas; para eso
  hace falta una integración de tareas (`tasks.json`) con el comando de gcc.
- `get_symbol_lsp_info` y `get_references` funcionan solo con símbolos que el language
  server C pueda resolver; no se usaron como criterio de calidad.

## 8. Correcciones aplicadas

Todas verificadas con los flags estrictos + `-Werror` y con la suite de pruebas.

| # | Corrección | Archivos |
|---|------------|---------|
| 1 | Includes no usados eliminados | `lcg_lehmer.c` (`stdio.h`, `stdlib.h`, `string.h`), `msws64.c` (`string.h`), `matematica_ancestral.c` (`stdio.h`, `stdlib.h`), `cordic.c` (`stdlib.h`) |
| 2 | Prototipos declarados | `cordic.h` (`cordic_radians_to_fixed16`, `cordic_fixed16_to_radians`), `lcg_lehmer.h` (`msws64_pure_next`), `matematica_ancestral.h` (`inverso_modular`) |
| 3 | Signo y truncamientos corregidos | `goertzel_test.c` (4), `hastings.c` (12 `malloc`/`calloc` con cast a `size_t`), `msws64.c` (2), `hybrid_prng.h` (4), `lcg_lehmer.c` (3 `__uint128_t`→`uint64_t`), `matematica_ancestral.c` (2) |
| 4 | `const` preservado | `hastings.c` — el campo `hastings_poly_t.coeffs` ya era `const double *`; se eliminó el cast que descartaba `const` |
| 5 | Truncamiento latente eliminado | `matematica_ancestral.c` — `fracciones_egipcias` trabaja con `int64_t` internos; el `int` intermediate truncaba el residuo silenciosamente |
| 6 | Extensión de 128 bits con `-Wpedantic` | `lcg_lehmer.c` — `__extension__ typedef` para `__int128`/`unsigned __int128` (GCC), silenciando el aviso ISO C sin cambiar la aritmética modular |
| 7 | Constantes K de Q15 corregidas | `cordic.h` (`CORDIC_FIXED16_K`: 54033 → 53961), `cordic_arduino.h` (`CORDIC_K_Q15`: 53960 → 53961). Valor exacto: K·32768 = 53961.04 tanto para N=12 como para N=16; `cordic.h` además no usaba la constante en ningún sitio |

### Pendiente (no aplicado)

- Los 8 avisos de estilo de cppcheck (sección 5.5) son cosméticos.
- `msws64.c` conserva `<stdint.h>` aunque cpptools lo marque: el TU usa `uint64_t`
  directamente y la norma exige incluir el header del tipo usado.
- `cordic.c` conserva `<math.h>` aunque cpptools lo marque: `ldexp` lo exige.
- Los sketches `.ino` siguen sin cobertura de diagnósticos (ver sección 7).
