# Análisis estático — Ternario-ancestral-sensores (Tritos OS)

**Repo analizado**: `https://github.com/Astrid3333/Ternario-ancestral-sensores.git`
**Clon local (solo lectura)**: `/home/astrid/Ternario-ancestral-sensores`
**Rama/HEAD**: `master` @ `f91a669` ("Graphics console + visible PS/2 mouse cursor")
**Fecha**: 2026-11-08
**Instrumentos**: VS Code MCP (bridge `yutengjing.vscode-mcp-bridge` 4.9.5 + Pylance 2026.4.1 + extensión Ruff + cpptools 1.35.6) vía **MCP Inspector** 2.13.0, contrastados con gcc 13.3.0 (`-m32`), ruff 0.15.19 en CLI, GNU Octave y ejecución directa.

> No se modificó ningún archivo del repo analizado (el único archivo escrito fue un probe de control, eliminado después).

---

## 1. Alcance y cobertura

| Superficie | Volumen | Analizado con |
|---|---|---|
| `output/ternary-kernel/src/*.c` + `boot/*.asm` + `include/ternary.h` | 45 `.c`, 20 413 líneas | gcc `-m32` (flags del proyecto + flags estrictos), cpptools vía MCP |
| `output/tak-userspace/src/*.c` + `include/*.h` | 29 `.c`, 20 910 líneas | gcc `-std=c99` (flags del Makefile) |
| Python raíz (`tritos_*.py`, `apply_*.py`, `test_*.py`) + `tritos_lab/` | 13 archivos (11 raíz + 2 en `tritos_lab/`) | Pylance + Ruff (MCP) + ruff CLI + `py_compile` + ejecución |
| `output/experiment-suite/**` | 34 `.py`, 10 `.m` | ruff CLI + Octave (ejecución real de los `run_all.m`) |
| Scripts shell | 5 `.sh` | `bash -n` |
| Arduino | 2 `.ino` | lectura (sin `arduino-cli` en el entorno) |
| Build system | `Makefile` (kernel y userspace), `build-kernel.sh`, `build-tritos-distro.sh`, `install.sh`, `create-usb.sh` | `make` real en copia temporal |

**Cobertura de compilador C**: 45/45 `.c` del kernel compilan (19 sin avisos, 26 con avisos, 0 fallos). Los 6 binarios del userspace compilan y linkean.

## 2. Bugs reales confirmados (verificados por ejecución o por el compilador)

### B1 — `tritos_gui.py`: `sys` nunca importado → NameError en 7 usos
`tritos_gui.py` importa `gi, subprocess, os, math, struct, wave, datetime, time` (líneas 4-13) pero **no** `sys`. Se usa en 2097, 2117 (×2), 2118, 2781, 2796 → `sys.executable`.
**Evidencia**: AST del módulo (`sys` importado=False, asignado=False, usos=[2097, 2117, 2117, 2118, 2781, 2796]) + dos analizadores independientes vía MCP: Ruff `F821` y Pylance `reportUndefinedVariable`.
**Impacto**: los botones "abrir navegador", "arrancar subproceso" y "buscar en la enciclopedia" fallan con `NameError` al pulsarlos.

### B2 — `tritos_gui.py:2388`: `disk.img` en vez de `disk_img` → NameError
```python
2382:  disk_img = os.path.join(script_dir, "disk.img")
2388:  cmd = ["qemu-system-i386", "-drive", f"file={disk.img}"]
```
**Evidencia**: Ruff `F821` + Pylance `reportUndefinedVariable` en 2388 (0-based 2387); `disk` no existe en el ámbito del módulo ni de la función (AST).
**Impacto**: el botón "correr kernel en QEMU" falla siempre. **Bug secundario**: busca `disk.img` en la raíz del repo, pero el build lo escribe en `output/releases/disk.img` → aunque se corrigiera el nombre, la ruta no coincide con lo que produce `make`.

### B3 — `src/fs.c`: el marcador de borrado FAT16 (0xE5) nunca coincide → el borrado de archivos está roto
`fat16_dir_entry_t.name` es `char name[8]` (**con signo** en x86). El marcador FAT16 de entrada borrada es 0xE5 → como `char` vale -27, así que `entries[i].name[0] == 0xE5` **es siempre falso** (gcc `-Wtype-limits`).
**Sitios**: `fs.c:196, 270, 441, 484, 632, 688` (6).
**Evidencia** (reproducción en host):
```
A) name[0] byte=0xE5 como char=-27  == 0xE5 ? NO (nunca coincide)
```
`fs_delete` marca la entrada escribiendo 0xE5 (`fs.c:642`), pero listar/abrir/medir nunca saltan las entradas borradas → **los archivos borrados siguen apareciendo y se pueden abrir**.

### B4 — `src/usb_storage.c:86`: escritura 1 byte fuera de rango que corrompe `mounted`
```c
34:  char product[16];
...
85:  for (int i = 0; i < 16; i++) product[i] = data[16 + i];
86:  product[16] = 0;          // ← 1 byte más allá de product[16]
```
El campo siguiente en la struct es `uint8_t mounted` → la escritura lo pone a 0.
**Evidencia**: gcc `-Wtype-limits` no lo marca (es OOB, no comparación), pero la reproducción en host muestra la corrupción: `mounted antes=1 → después=0`.
**Bug asociado**: `uint8_t data[36]` (línea 75) se lee sin inicializar cuando la transferencia SCSI falla parcialmente → gcc: `'data' is used uninitialized` (línea 83).

### B5 — `tritos_lab/tritos_godot.py:98`: SyntaxError → módulo no importable
```python
98:  [self.godot", "--headless", "--path", project_dir],
```
Cadena sin cerrar. **Evidencia**: `python3 -m py_compile` → `SyntaxError: unterminated string literal`; Ruff `invalid-syntax` (8 errores) y Pylance `"[" was not closed`.

### B6 — `tritos_lab.py` (módulo) sombrea al paquete `tritos_lab/` → integraciones Blender y Godot muertas
`tritos_lab.py` y el directorio `tritos_lab/` conviven en la raíz. En Python el módulo `.py` gana al namespace-package, por lo que `tritos_lab.tritos_blender` no existe.
**Evidencia** (ejecución en la raíz del repo):
```
tritos_lab.__file__ = /home/astrid/Ternario-ancestral-sensores/tritos_lab.py
tritos_blender FAIL: ImportError cannot import name 'tritos_blender' from 'tritos_lab'
tritos_godot  FAIL: ImportError cannot import name 'tritos_godot' from 'tritos_lab'
```
Afecta a `tritos_lab.py:906` (`from tritos_lab.tritos_blender import TritosBlender`) y `tritos_lab.py:1012` (`from tritos_lab.tritos_godot import TritosGodot`) → las dos integraciones del lab fallan en tiempo de ejecución (y quedan ocultas por un `except` genérico). Con B5, Godot además no compila.

### B7 — `src/libc_full.c:342-346`: `getchar()` devuelve un valor no inicializado
```c
int getchar(void) {
    char c;
    syscall_read(0, &c, 1);
    return c;      // gcc: 'c' is used uninitialized
}
```
Si la lectura no completa 1 byte, se devuelve basura. Además `char` (con signo) en vez de `int` rompe la semántica de EOF (-1).

### B8 — `num_to_str` no acota el destino → desbordamientos sistemáticos
`include/ternary.h:273` escribe dígitos + NUL sin límite de tamaño. Hay **41 llamadas con `char nb[2]`** (10 archivos: `phase4.c` 24, `user.c` 6, `usb.c` 4, `dl.c`, `elf.c`, `mail.c`, `parental.c`, `sockets.c`, `usb_storage.c`, `vpn.c` 1 cada uno) cuyo valor puede superar 9:
`sockets.c:132` (`fd`), `dl.c:168` (`i`), `user.c:315/317/334` (`uid`/`gid`/`member_count`), `usb.c:211/251/256` (`usb_device_count`), `phase4.c:513-532` (`kleene_and/or`,bounded) → 1-2 bytes de desbordamiento de pila.
Además `usb_storage.c:163-167`: `char nb[4]` + `num_to_str(idx, nb)` desborda con `idx ≥ 1000`, y el `while (nb[len])` puede leer fuera del buffer (gcc: `'nb' may be used uninitialized`).

### B9 — 5 declaraciones implícitas de funciones (prototipos ausentes)
| Archivo:línea | Función | Definición real |
|---|---|---|
| `src/elf_dynamic.c:238` | `mem_alloc` | `int16_t mem_alloc(uint8_t, uint8_t)` (`src/memory.c:61`) |
| `src/scheduler.c:95` | `mem_free` | `int8_t mem_free(uint8_t)` (`src/memory.c:81`) |
| `src/kernel.c:63`, `src/syscalls.c:91` | `serial_putc` | — |
| `src/kernel.c:1549` | `wm_redraw` | — |

La declaración implícita asume retorno `int`, incompatible con `int16_t`/`int8_t` reales → comportamiento indefinido según el estándar y error duro en C23 (gcc 14+ lo ya trata así).

### B10 — `src/net.c:202,236`: cast de puntero a entero de tamaño distinto
`warning: cast from pointer to integer of different size` (aparece incluso con `-m32`). Riesgo de truncamiento según el tipo destino.

### B11 — Guardas de rango que nunca se activan por tener el contador demasiado corto
Mismo mecanismo que B3 (`-Wtype-limits`), pero con contador `uint8_t`:
| Archivo:línea | Código | Tipo real | Constante | Consecuencia |
|---|---|---|---|---|
| `src/elf_dynamic.c:133` | `if (global_symbol_count >= MAX_SYMBOLS) return -1;` | `uint8_t global_symbol_count` (`elf_dynamic.c:125`) | `MAX_SYMBOLS 256` | La guarda nunca dispara: al cargar el símbolo nº 256 se escribe **fuera de `global_symbols[256]`** (desbordamiento global) |
| `src/scheduler.c:123` | `if (scheduler.tzolkin_day > TZOLKIN) { … }` | `uint8_t tzolkin_day` (`scheduler.c:20`) | `TZOLKIN 260` | El día tzolkin **no rota en 260**: desborda a 256 → el calendario maya de 260 días queda roto a partir del día 256 |
| `src/scheduler.c:128` | `if (scheduler.haab_day > HAAB) { … }` | `uint8_t haab_day` (`scheduler.c:21`) | `HAAB 365` | El día haab desborda a 256 en vez de rotar en 365 |

### B12 — Mezcla de dos modelos de coste de Landauer en el artefacto científico
`output/experiment-suite/01-ternario-ancestral-sensores/experiment/run_all.m:88-94`:
```matlab
88: E_binary   = kB*T*log(2);   % 2.871e-21 J  (por bit)
89: E_ternary  = kB*T*log(3);   % 4.550e-21 J  (por trit)
92: fprintf('Eficiencia ternario/binario: %.3f\n', log(3)/log(2));   % -> 1.585
93: fprintf('ln(3)/3 = %.6f vs ln(2)/2 = %.6f\n', ...);            % -> 0.366204 vs 0.346574
94: fprintf('Ternario es %.1f%% más eficiente por símbolo\n', ...); % -> 5.7%
```
Las líneas 88-89 usan el coste de Landauer **por símbolo** (energía mínima para borrar un símbolo de S estados equiprobables: kB·T·ln S); la 94 compara **ln(S)/S**, que es otro modelo (memoria multinivel). Con las dos líneas juntas, la salida dice "Eficiencia ternario/binario: 1.585" (ratio de energía por símbolo → el ternario **gasta 58 % más** por símbolo) y a continuación "Ternario es 5.7 % más eficiente por símbolo" (conclusión opuesta).
En unidades homogéneas: por símbolo, ternario = 4.550e-21 J vs binario = 2.871e-21 J (1.585×); **por bit de información** ambos dan kB·T·ln2 → 0 % de diferencia. La ventaja del 5.7 % solo se obtiene mezclando normalizaciones (÷3 para el ternario y ÷2 para el binario, cuyo símbolo ya es un bit).
**No afecta** al resultado de compresión (665.86 bits ternario vs 8048 binario → 86.9 % de ahorro energético), que es consistente con lo ejecutado.

## 3. Deuda de build y documentación

| # | Hallazgo | Evidencia |
|---|---|---|
| D1 | `build-kernel.sh` compila y linkea **3 de 45** TUs (`kernel.c`, `memory.c`, `scheduler.c`, líneas 71-78 y 104) mientras el `Makefile` sí construye los 45 | `build-kernel.sh:71-78`, `Makefile` (45 reglas `.o`) |
| D2 | El ISO que genera `build-kernel.sh` usa `linux16 /boot/ternary.img` (línea 134) para una imagen **multiboot** → ese ISO no arranca; el `Makefile` lo hace bien con `multiboot /boot/ternary_kernel.elf` | `build-kernel.sh:128-141` vs `Makefile` (grub.cfg) |
| D3 | Versionado divergente: el script anuncia "Kernel v2" y el Makefile "v4.5" | `build-kernel.sh:132` vs grub.cfg del Makefile |
| D4 | `AGENTS.md` declara 20+ componentes del kernel (fs, net, tls, process, syscalls…) como "✅"; el artefacto que produce `build-kernel.sh` solo contiene 3 TUs | `AGENTS.md:41-62` vs D1 |
| D5 | La GUI busca `disk.img` en la raíz del repo; el build lo produce en `output/releases/disk.img` | `tritos_gui.py:2382` vs Makefile |

## 4. Lint y avisos (no bloqueantes)

- **Python raíz + `tritos_lab/`**: 51 hallazgos ruff (`F541` 24, `F401` 10, `F821` 7, `F841` 1, `F402` 1) más `E402`/`E701`/`E702`/`E722`/`E401`. `tritos_math_panel.py:168` usa `for sys in [...]` **sombreando el módulo `sys`** (F402).
- **`output/experiment-suite/`**: 22 hallazgos ruff (solo `F401`/`F841`, sin errores de ejecución).
- **C (kernel, flags del proyecto)**: **89 avisos en 27 archivos** — `unused-variable` 25, `unused-function` 15, `unused-parameter` 12, `type-limits` 9, `sign-compare` 6, `implicit-function-declaration` 5, `unused-but-set-variable` 2, `uninitialized` 2, `pointer-to-int-cast` 2, `maybe-uninitialized` 2. Los 9 `type-limits` son B3 (6 en `fs.c`) y B11 (1 en `elf_dynamic.c`, 2 en `scheduler.c`).
- **Falsos positivos clasificados**: `Pylance reportAttributeAccessIssue` sobre `gi.repository.*` (Gtk/Gdk/GLib/Pango/WebKit2) → faltan los stubs de PyGObject en el entorno, no es un defecto del repo. `tritos_gui.py:376` (`self.start_menu.hide()` sobre un `Optional`) → el menú se asigna en la línea 392, antes de que el lambda pueda ejecutarse.

## 5. Lo que funciona (verificado)

| Componente | Resultado |
|---|---|
| Kernel (`make` en `output/ternary-kernel`) | ✅ 45 TUs → `ternary_kernel.bin` 315 137 B, `ternary_os.iso` 12,5 MB, `disk.img` 315 KB |
| Userspace (`make` en `output/tak-userspace`) | ✅ 6 binarios (shell 292 KB, science 148 KB, ai 48 KB, browser 28 KB, terminal 40 KB, compress 28 KB) |
| Scripts shell | ✅ `bash -n` sin errores en los 5 |
| Experimento 01 (sensores, Octave) | ✅ `run_all.m` termina con "TODO MEDIDO OK" (8048 bits binario vs 665.86 ternario, 86.9 % de ahorro) |
| Experimento 04 (calendarios, Octave) | ✅ verificaciones propias pasan; conversiones correctas (13.0.0.0.0 → JDN 2456283; correlación GMT 584283) |
| `.m` de función (5) | ✅ parsean; los errores al invocarlos sin argumentos son de ejecución, no de sintaxis |

**Gaps de entorno** (no defectos del repo): sin `shellcheck`, sin `arduino-cli`, sin `genisoimage` (el `Makefile` cae correctamente en `grub-mkrescue`, que sí funciona).

## 6. Metodología: qué aporta cada instrumento

| Instrumento | Aportación | Límite |
|---|---|---|
| **MCP Inspector** 2.13.0 | Auditar el contrato del conector (8 tools, esquemas, `required`, anotaciones `readOnlyHint`/`execution.taskSupport`) y ejecutar `health_check`/`list_workspaces` de forma reproducible | No es un analizador de código: sirve para validar el canal MCP y el contrato de tools |
| **VS Code MCP + Pylance/Ruff** | B1, B2 (doble confirmación Ruff + Pylance), B5, lint Python | Solo reporta **archivos abiertos**; hay que abrirlos antes (12 Python + 4 C + 1 probe de control en esta ronda) |
| **cpptools (IntelliSense)** | Confirmó que no hay errores semánticos en `fs.c`, `usb_storage.c`, `libc_full.c` | **No detecta** B3, B4, B7 ni B8: solo análisis semántico, no de compilador. Verificado con archivo de control (`_cpptools_probe_tmp.c`, eliminado): sí detecta identificadores no definidos → el "clean" era un negativo real, no un fallo del instrumento |
| **gcc `-m32` con flags del proyecto** | Lo que sostiene el análisis C: B3, B4, B7, B8, B9, B10 | Requiere `gcc-multilib` (presente) |
| **ruff CLI** | Recuento global y detección de `F821` en archivos no abiertos | No distingue nombres redefinidos de nombres ausentes en runtime |
| **Octave** | Ejecución real de los artefactos científicos → B11 es verificable, no una inferencia | No cubre los `.py` de los experimentos (no instalados) |

Las posiciones que devuelve `get_diagnostics` son **0-based** (API de VS Code): en este informe todas las líneas citadas son 1-based reales, verificadas con `grep -n`, `py_compile` y gcc.

## 7. Recomendaciones priorizadas

1. **B1/B2** — `tritos_gui.py`: añadir `import sys` y corregir `disk.img` → `disk_img` (+ alinear la ruta con `output/releases/`). Impacto inmediato en la GUI principal.
2. **B5/B6** — `tritos_lab/`: renombrar el módulo `tritos_lab.py` (p. ej. `tritos_lab_ui.py`) o convertir el directorio en paquete con `__init__.py`, y corregir la cadena rota de `tritos_godot.py:98`. Devuelve Blender y Godot al lab.
3. **B3** — `fs.c`: comparar contra `(char)0xE5` o cambiar `name`/`ext` a `uint8_t`. Sin esto, `fs_delete` no borra nada observable.
4. **B4/B8** — `usb_storage.c`: `memset(data, 0, sizeof data)`, `product[15]` como terminador o `char product[17]`, y `num_to_str` con parámetro de tamaño.
5. **B7/B9/B10** — prototipos en `include/ternary.h` (`mem_alloc`, `mem_free`, `serial_putc`, `wm_redraw`), `getchar()` con `int` + valor de retorno de `syscall_read`, y casts de puntero explícitos en `net.c`.
6. **B11** — ampliar a `uint16_t` los contadores `global_symbol_count` (`elf_dynamic.c`), `tzolkin_day` y `haab_day` (`scheduler.c`), o cambiar las guardas a `>=` con el límite real; hoy las tres son inoperantes.
7. **B12** — unificar el modelo de coste en `run_all.m`: reportar por símbolo (kB·T·ln S) y por bit (kB·T·ln2) por separado, sin mezclar `ln(S)/S`.
8. **D1/D2** — dejar `build-kernel.sh` como wrapper de `make` (o marcarlo como obsoleto) para no volver a construir un kernel de 3 TUs con un ISO que no arranca.
9. **D4** — alinear `AGENTS.md` con lo que el build produce realmente.

## 8. Reproducibilidad

```bash
# compilación estricta del kernel (los 45 .c)
cd output/ternary-kernel
CFLAGS="-m32 -ffreestanding -O2 -Wall -Wextra -nostdlib -nostdinc -fno-builtin \
        -fno-stack-protector -nostartfiles -nodefaultlibs -fno-pie -no-pie"
for f in src/*.c; do gcc $CFLAGS -Iinclude -c "$f" -o /tmp/$(basename ${f%.c}).o; done

# build completo (en copia, para no tocar .o versionados)
cp -r output/ternary-kernel output/tak-userspace output/releases /tmp/t/ && make -C /tmp/t/ternary-kernel

# lint Python
ruff check --select F,E9 --no-cache $(find . -name '*.py' -not -path './.git-rewrite/*')

# contrato del conector vía MCP Inspector
mcp-inspector-vscode --cli --method tools/list
```

Evidencia archivada en este directorio: `mcp_inspector_tools.json` (contrato de las 8 tools del conector).
