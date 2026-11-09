# Current State

- Goal: entregar análisis estáticos de repos vía VS Code MCP + MCP Inspector.
- Stage: ciclo 5 (`mcp-ecosystem-chiloe`, análisis declarado vs. MCP real) — **cerrado**: fixes
  de código, entorno y despliegue aplicados y re-verificados; ciclo 4 (Ternario) cerrado antes.
- Current task: ciclo 5 cerrado. Estado medido post-corrección (2026-11-09 14:26, MCP Inspector
  2.13.0): template **21/24 responden, 250 tools**; config desplegada **21/22, 734 tools** (antes
  17/28 · 196 y 9/12 · 639). Informe con §7 de cierre en
  `analysis/mcp-ecosystem-chiloe/ANALISIS_VS_MCP.md`, evidencia en
  `analysis/mcp-ecosystem-chiloe/mcp_inspector_tools.json`, captura reproducible en
  `analysis/mcp-ecosystem-chiloe/capture_tools.py`. Fixes: A1 (`import os` restituido),
  A2 (`id` correlacionable), A3 (nuevo `octave_runner.py` compartido), A4 (validaciones +
  `digits` funcional), B2 (scipy 1.18.1 reinstalado en los 2 venvs), B1/B4 (template a la ruta
  canónica + podas + npx real), B3 (10 servidores agregados a la config desplegada).
- Next action: con el usuario — decidir si el `octave` viejo del deploy (3 tools) se reemplaza
  por `octave-chiloe` (13 tools), y si `bench`/`mcp-blender-full`/`unrealMCP` entran al
  template. Pendiente known: auditar los 12 archivos del WIP del usuario (C3) y los 3
  servidores `uvx` que requieren red (`gdal`, `openbb-mcp`, `qgis`).
- Ciclo 4 (cerrado): las 12 correcciones de Ternario están aplicadas y commiteadas en
  `Ternario-ancestral-sensores` (`master` @ `b361241`, fix de código en `544f633`, pusheado).
  Verificado: gcc `-m32 -Wall -Wextra` sobre las 45 TUs del kernel con 0 errores y con las
  clases de aviso que eran bug real en 0 (`-Wtype-limits` 9→0, `-Wimplicit-function-declaration`
  5→0, `-Wuninitialized`/`-Wmaybe-uninitialized` 2→0, `-Wpointer-to-int-cast` 2→0; total
  89→68, el resto `unused-*`/`sign-compare`); `make all` produce ISO + disk.img y el kernel
  bootea en QEMU; el userspace compila; los 2 experimentos Octave corren; `py_compile` OK y
  ruff sin hallazgos nuevos. El informe se actualizó con la sección "0. Estado tras la corrección".

## Facts_changed

- `mcp-ecosystem-chiloe`: la copia **canónica** es
  `/home/astrid/Documents/OpenScience/projects/githu/mcp-ecosystem-chiloe` (ahora `982ab0f`,
  pusheado a `origin/main`); la copia **obsoleta** `/home/astrid/mcp-ecosystem-chiloe`
  (`c022adf`, 7 commits atrás) expone **1 tool** frente a **13** de la canónica. El template
  (working tree, con WIP del usuario) **ya apunta a la canónica** → no volver a la obsoleta.
- Estado real medido con MCP Inspector post-corrección (2026-11-09 14:26): template **21/24**
  con **250 tools**; `~/.config/Claude/claude_desktop_config.json` **21/22** con **734 tools**.
  Los 3 que no responden (template) y 1 (desplegada) son `uvx` que necesitan red: `gdal`,
  `openbb-mcp`, `qgis` — no son bugs del repo.
- Config desplegada: 12 → **22** servidores (backup `claude_desktop_config.json.bak-20261109-142058`).
  Se agregaron los 9 servidores propios del repo + `octave-chiloe` (13 tools) apuntando a la
  ruta canónica; **sin tocar** las 12 entradas previas. El `octave` viejo (3 tools,
  `/home/astrid/octave-mcp/server.py`) se conservó a propósito.
- Template: 28 → **24** claves (podadas `mcp-registry`, `inkscape`, `krita`,
  `materials-project` — binarios inexistentes — y **2 claves duplicadas** `analisis-suelos` y
  `bolsa-mcp`, cuya versión vieja pisaba a la nueva). Rutas `npx` corregidas al node real
  (`/home/astrid/.local/share/pi-node/node-v22.23.2-linux-x64/bin/npx`): `filesystem` pasó de
  caído a 14 tools y `firecrawl-mcp` de caído a 26 tools.
- `microbio-env` y `physics-env`: scipy **arreglado** (1.18.1, reinstall `--only-binary=:all:`;
  los `.so` previos no eran ELF, magic `00 10 dd fc`). `microbio-mcp` = 6 tools y
  `physics-mcp` = 8 tools → ambos vuelven a responder en las dos configs.
- `octave_mcp.py` ahora importa `run_octave` del nuevo módulo compartido
  `servidores-propios/octave_runner.py` (`_octave_bin()` con `shutil.which`); se eliminó el
  shim `sys.modules["server"]` y el import frágil `from server import run_octave`.
- El `octave` desplegado (`/home/astrid/octave-mcp/server.py`) **solo arranca con su venv**
  (`/home/astrid/octave-mcp/.venv/bin/python`, handshake 5.6 s); con `python3` del sistema muere
  con `ModuleNotFoundError: No module named 'ocas'` (`server.py:92` → `ocas_symbolic_tool.py:24`).
  Claude Desktop lo lanza con el venv → funciona, pero expone 3 tools (otro repo, 214 módulos).
- MCP Inspector CLI (2.13.0) — tres trampas verificadas: (1) **resuelve el target contra su cwd**
  (cwd = dir del script + ruta absoluta → duplica el prefijo y el server no arranca); (2) **en
  paralelo devuelve stdout vacío** (compiten por el catálogo compartido) → correr secuencial;
  (3) su `tools/list` devuelve `{"tools": [...]}` **sin** envoltura JSON-RPC → un parser que
  exija `result`/`error` marca como caído un server sano.
- Disco de la máquina: de 14 G a **17 G libres (84 %)** tras borrar cachés regenerables
  (`~/.cache/uv` 1.3 G, xdg-cache del workbench ~630 M, BraveSoftware, `__pycache__` 91 M,
  basura PostScript `gc/oo/sys`). No se tocó `opencode.db` (3.2 G), extensiones de Blender
  (655 M), `octave-mcp/.venv` (1.3 G) ni `.venv-pyscf` (438 M).
- Working tree de `mcp-ecosystem-chiloe`: el WIP del usuario queda **intacto y sin commitear**
  (incluye `verificar_mcp.py`, `bolsa_mcp.py`, `claude_desktop_config.template.json`,
  `jobs.py`, `copasi_mcp.py`, `gazebo_*.py`, `mcp_suelos.py`, `hilbert_tool.py`, `btfloat.py`).
  El commit `982ab0f` incluye **solo** 4 archivos propios (`octave_runner.py` nuevo,
  `octave_mcp.py`, `middle_square_weyl_tool.py`, `ancestral_octave_tool.py`) → patrón
  `git commit --only <paths>` para no arrastrar WIP ajeno.
- El WIP del usuario reintroducía la copia obsoleta en la ruta de `octave`; **corregido** en el
  working tree del template, pero **sin commitear** (queda mezclado con el WIP del usuario).
- `verificar_mcp.py`: el `import os` que el WIP había borrado (y que HEAD sí tenía) fue
  restituido → el archivo vuelve al estado de HEAD en ese punto y no genera diff.

- `Ternario-ancestral-sensores`: `master` local y remoto @ `b361241` (antes `f91a669`).
  Commit `544f633` = los 12 bugs (B1–B12) + deuda D1/D2; `b361241` = memoria del ciclo.
  Rama `fix/analisis-vs-mcp` también pusheada. El WIP del usuario (GUI del kernel sobre
  framebuffer: `window.c` +198, `kernel.c` con `gui_run()`, prototipos `gui_*` en `ternary.h`,
  `grub.cfg` reordenado, `boot/boot.bin` borrado, `multiboot_header.o` modificado y 2 `.py`
  sin trackear) quedó **sin commitear, sin cambios y fuera del remoto**.
- Bugs extra hallados al corregir Ternario (no estaban en el informe): `build-kernel.sh`
  pisaba el `linker.ld` versionado con uno incompatible (`ENTRY(kernel_main)`, `0x10000`, sin
  `.multiboot`) y usaba `linux16` para una imagen multiboot; `make clean` borraba archivos
  versionados (`boot/boot.bin`, `boot/multiboot_header.o`, `grub.cfg`); `make all` fallaba en
  un clon limpio (no creaba `output/releases/`); el `Makefile` reescribía el `grub.cfg`
  versionado en cada build.
- Corrección de B8: `num_to_str_bounded`/`int_to_str_bounded` en `include/ternary.h` y
  **355 llamadas** migradas en 24 archivos (41 buffers `char nb[2]`); además 20 llamadas en
  `kernel.c` con `char num_buf[8]` (11 bytes para un `uint32_t`).
- `Ternario-ancestral-sensores` está clonado en `/home/astrid/Ternario-ancestral-sensores`
  (`master` @ `f91a669`); es el OS "Tritos" y tiene su propio `AGENTS.md`/`KNOWLEDGE.md`/
  `notes/`. Análisis de solo lectura (único archivo escrito: probe de control, eliminado).

- Bugs de Tritos verificados (y ya corregidos en `544f633`): `tritos_gui.py` usaba `sys` sin
  importarlo (7 usos) y `disk.img` en vez de `disk_img` (2388); `tritos_lab/tritos_godot.py:98`
  tenía un SyntaxError; `tritos_lab.py` sombreaba al paquete `tritos_lab/` (ImportError en
  Blender y Godot); `fs.c` comparaba 0xE5 contra `char` con signo (6 sitios → el borrado FAT16
  era inoperante); `usb_storage.c:86` escribía `product[16]` en `char product[16]` (corrompía
  `mounted`); `libc_full.c:345` devolvía `char` sin inicializar; `num_to_str` (`ternary.h:273`)
  no acotaba el destino y había 41 llamadas con `char nb[2]`; `elf_dynamic.c:133` y
  `scheduler.c:123,128` tenían guardas que nunca se disparaban (contador `uint8_t` vs límites
  256/260/365); `run_all.m:88-94` mezclaba dos modelos de coste de Landauer (imprimía 1.585 y
  "5.7 % más eficiente" a la vez).
- `build-kernel.sh` estaba obsoleto frente al `Makefile` (compilaba 3 de 45 TUs, ISO `linux16`
  para una imagen multiboot y pisaba el `linker.ld` versionado): **corregido**, ahora delega en
  el `Makefile` canónico.
- Barrido con gcc en `Ternario-ancestral-sensores`: antes 89 avisos en 27 archivos del kernel
  (9 `type-limits`, 5 `implicit-function-declaration`, 2 `uninitialized`, 2 `maybe-uninitialized`,
  2 `pointer-to-int-cast`); después de los fixes **68 avisos, 0 errores** y las 4 clases de bug
  real en 0 (los 68 restantes son `unused-variable/function/parameter` y `sign-compare`).
  Los `.py` de la raíz dan 51 hallazgos ruff y los del `experiment-suite` 22 (de estilo, 0 nuevos).


- `octave-mcp` está clonado en `/home/astrid/octave-mcp` (rama `migracion-mcp-octave-real`,
  HEAD `5d84431`); `git fetch` por SSH funciona. El informe vive en el workspace propio
  (`githu`) para no ensuciar ese repo con archivos sin trackear.
- `server.py` de `octave-mcp` importa explícitamente 214 módulos tool de la raíz; los
  subdirectorios `tools/` (91 `.py`) y `submotors/` (6 `.py` de 1210 archivos) **no** los
  carga (no hay descubrimiento dinámico). Las copias en `tools/` son versiones divergentes
  de la raíz, no duplicados exactos.
- Bugs reales en `octave-mcp`: `plotting_tools.py:523,540` usa `plt` sin importarlo
  (NameError alcanzable); `tools/ion_chemistry_tool.py:667` usa `TOOL_SCHEMA` inexistente
  (NameError al importar, verificado); `tools/__init__.py` no importa `tool_registry` y
  registra dos veces la misma tool; `validate` duplicado en 2 archivos de `tools/`;
  bloques `__main__` rotos en 5 tools legacy.
- El smoke test de `octave-mcp` no es reproducible en este entorno: faltan 7 de las 15
  dependencias de `requirements.txt` (`ocas`, `opencv-python-headless`, `scikit-image`,
  `okada_wrapper`, `cypari`, `python-flint`, `symengine`), por lo que `import server` falla.
  El CI sí las instala, luego es gap de entorno y no defecto del repositorio.
- `vscode_get_diagnostics` solo devuelve diagnósticos de **archivos abiertos en el editor**
  (para Python, un archivo no abierto devuelve "clean" falso). Las posiciones que devuelve
  son **0-based** (API de VS Code): hay que sumar 1 para citar la línea real.
- El bridge VS Code MCP requiere que VS Code se lance con `XDG_DATA_HOME=/home/astrid/.local/share`
  (igual que el servidor MCP en `~/.config/opencode/opencode.json`); con el `XDG_DATA_HOME` largo del
  workbench el path del socket Unix supera los 107 caracteres y el bind falla.
- En esta máquina, VS Code necesita `--disable-gpu` para abrir ventana (sin él no arranca el
  renderer y el extension host no levanta); tras un `kill -9` hay que borrar el `code.lock`
  obsoleto del perfil. El extension host tarda 2-3 min en arrancar: el MCP puede parecer caído.
- `codigos-potenciados` no es un repo git propio: es un subdirectorio del repo `githu`
  (raíz: `/home/astrid/Documents/OpenScience/projects/githu`).
- VS Code tiene ahora `ms-vscode.cpptools` v1.35.6 instalado; el análisis C/C++ real requiere
  además `.vscode/c_cpp_properties.json` + `compile_commands.json` generado por
  `codigos-potenciados/tools/gen_compile_commands.py`.
- En Linux cpptools no propaga warnings del compilador al panel de Problemas (solo semántica
  IntelliSense); los warnings de gcc hay que obtenerlos aparte.
- MCP Inspector v2.13.0 instalado global (`npm i -g @modelcontextprotocol/inspector`; binarios
  `inspector`/`mcp-inspector` en el prefix global de Node, ya en el PATH). El "conector"
  auditado es `/home/astrid/vscode-mcp/node_modules/.bin/vscode-mcp-server` (8 tools del
  puente de VS Code); atajo `~/.local/bin/mcp-inspector-vscode` (web por defecto, `--cli`/`--tui`).
- El escritorio (workbench) define `XDG_DATA_HOME` con un path largo
  (`/home/astrid/.local/share/com.ai4s.workbench/runtime/xdg-data`): los scripts que lanzan
  el conector deben **forzar** `XDG_DATA_HOME=/home/astrid/.local/share` (no usar
  `${XDG_DATA_HOME:-...}`, que hereda el valor del escritorio) o el socket Unix no funciona.
- Los sockets del puente (`~/.local/share/yutengjing-vscode-mcp/*.sock`) se borran cuando
  VS Code se cierra: `list_workspaces` con 0 resultados puede significar solo "VS Code cerrado".
