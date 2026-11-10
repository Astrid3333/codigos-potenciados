# Current State

- Goal: entregar análisis estáticos de repos vía VS Code MCP + MCP Inspector.
- Stage: ciclo 6 (auditoría del **WIP del usuario** en `mcp-ecosystem-chiloe`) — **cerrado**:
  12 hallazgos, **10 corregidos**, 1 cerrado a nivel de entorno (C3-12) y la corrida de humo **ejecutada
  con éxito vía MCP con el env de la app**; lo único abierto es la invocación **desde el chat de la UI**
  de Claude Desktop (bloqueada por cuota y por exposición de herramientas). Ciclo 5 (análisis declarado
  vs. MCP real) cerrado antes; ciclo 4 (Ternario) cerrado antes.
- Current task: ciclo 6 cerrado. Auditoría del WIP (11 archivos modificados + 12 untracked) en
  `analysis/mcp-ecosystem-chiloe/ANALISIS_WIP.md` + `wip_audit_evidence.json`. Hallazgos:
  **C3-1 alta** (`formulas_tool.py` sin versionar → el repo publicado no arrancaba en clon
  limpio; **corregido** en `065ed73`), **C3-2 alta** (`jobs.py` con `shell=False`+`shlex.split`
  rompe `run_gromacs`: el cmd lleva `&&`; **corregido** en `8bfd6ba`, opción A), **C3-3 media**
  (`gazebo_utils.py` sin versionar; **corregido** en `065ed73`), **C3-4 media** (COPASI: el bloqueo
  era el entorno, no la instalación; **corregido** en `8bfd6ba` + config con el venv), C3-5 baja
  (`job_id` colisiona; **corregido** en `d4cb051`), C3-6 baja (16 imports muertos; **corregido** en
  `d4cb051`), C3-7 baja (`_octave_bin()` duplicado ×3; **corregido** en `d4cb051`), C3-8 info
  (cambio de contrato JSON en `ejecutar_octave`; **corregido** en `e56a081`: ambos handlers devuelven
  `dict` con contrato documentado), C3-9 info (clave TLS
  sin regla de ignore; **corregido**), C3-10/C3-11 media (log real de GROMACS y falso positivo de
  `error`; **corregidos** en `8bfd6ba`), **C3-12 media** (`octave-chiloe` sano pero invisible para
  Claude Desktop/Inspector por los `XDG_*` del runtime del workbench; **corregido a nivel máquina**:
  registro en `~/.config/octave/api-v61/octave_packages` + paquetes en
  `~/.local/share/octave/api-v61/packages/`). Verificado bueno del WIP: timeout+cleanup de
  `mcp_suelos`, `hilbert_tool` con `mkstemp`, `bolsa_mcp` con dato real (AAPL 349,36),
  `gazebo-core` (`Gazebo Sim 8.15.0`), `copasi` operativo con la solución analítica.
- Next action: con el usuario — (a) reintentar la corrida de `run_gromacs` **desde el chat de Claude
  Desktop** cuando termine el límite de cuota (*"You are out of free messages until 3:30 AM"*) y confirmar
  si `md-simulation` aparece entre las herramientas del modelo (en la sesión probada no le fue expuesta,
  aunque el server está sano: handshake + `tools/list` OK), (b) reiniciar Claude Desktop para que
  `analisis-suelos` tome el `mcp_suelos.py` nuevo (`e56a081`), (c) pendiente known: los 3 servidores
  `uvx` que requieren red (`gdal`, `openbb-mcp`, `qgis` — `openbb-mcp` es el único que efectivamente
  cae en la sesión real).
  **Cerrado en este ciclo**: C3-2 (opción A), C3-5/6/7, C3-8, `copasi` con venv, `octave` viejo →
  `octave-chiloe`, `bench`/`mcp-blender-full`/`unrealMCP`/`md-simulation` en las configs, el WIP
  versionado (`3bf3141`) y la corrida de humo con el env de la app (job
  `gromacs_smoke_app_1794273194_0e4478`, `finished: true`).
- Ciclo 4 (cerrado): las 12 correcciones de Ternario están aplicadas y commiteadas en
  `Ternario-ancestral-sensores` (`master` @ `b361241`, fix de código en `544f633`, pusheado).
  Verificado: gcc `-m32 -Wall -Wextra` sobre las 45 TUs del kernel con 0 errores y con las
  clases de aviso que eran bug real en 0 (`-Wtype-limits` 9→0, `-Wimplicit-function-declaration`
  5→0, `-Wuninitialized`/`-Wmaybe-uninitialized` 2→0, `-Wpointer-to-int-cast` 2→0; total
  89→68, el resto `unused-*`/`sign-compare`); `make all` produce ISO + disk.img y el kernel
  bootea en QEMU; el userspace compila; los 2 experimentos Octave corren; `py_compile` OK y
  ruff sin hallazgos nuevos. El informe se actualizó con la sección "0. Estado tras la corrección".

## Facts_changed

- `mcp-ecosystem-chiloe` `origin/main` @ **`1208598`** (commit propio del usuario, 2026-11-09 23:50
  −0300): `jobs._pid_alive` da por **terminado** un proceso zombie (`os.waitpid(pid, WNOHANG)` + estado
  `Z` leído de `/proc/<pid>/stat`) y suma una regla de `.gitignore`. Incluye `e56a081` (C3-8) como
  ancestro. No auditado por el agente: fuera del alcance del informe, que cierra en `e56a081`.
- `mcp-ecosystem-chiloe` `origin/main` @ **`e56a081`** (push `42ab49a..e56a081`): **C3-8 corregido** —
  `servidores-propios/mcp_suelos.py`: `ejecutar_octave` y `analizar_suelo` devuelven `dict` (`ok`,
  `stdout`, `stderr`, `returncode`, `error` si aplica) con docstrings que fijan el contrato; se eliminó
  el `import json` muerto. Verificado por MCP (ok / timeout / error), `ruff` y `py_compile`.
- Corrida de humo de `run_gromacs` **con el env de la app** (`env -i` + `XDG_*` reales, MCP Inspector
  2.13.0, cwd `/tmp`): job `gromacs_smoke_app_1794273194_0e4478` en `/tmp/opencode/mdtest/run_app` →
  `status: finished`, `finished: true`, `had_error_keyword: false`, 3 bloques de energía,
  `203.969 ns/day`, log real en `topol.log`. Cubre C3-10 y C3-11 en la ruta de la app.
- Prueba desde la **UI de Claude Desktop** (2026-11-10 01:4x): el mensaje se envió y el modelo respondió,
  pero listó solo GitHub / bench / Claude Octave / Unreal / Claude Docs → **`md-simulation` no le fue
  expuesta**; sin `tools/call` en `mcp.log`, sin job `smoke_ui` y sin `run_ui/`; el composer muestra
  *"You are out of free messages until 3:30 AM"*. El server está sano: tras el reinicio de Claude Desktop
  (pid 125480, 01:45:32) levantan los 24 servidores y `md-simulation` completa initialize → `tools/list`
  → result sin errores ni warnings.

- `mcp-ecosystem-chiloe` `origin/main` @ **`42ab49a`** (push `d4cb051..42ab49a`): `d4cb051` = C3-5
  (`job_id` con sufijo `uuid4().hex[:6]`), C3-6 (16 imports muertos vía `ruff --select F401 --fix`;
  `ruff check --select F401,F811,F841,E9` → *All checks passed!*) y C3-7 (`octave_runner.octave_bin()`
  público, alias `_octave_bin`; `hilbert_tool.py` y `mcp_suelos.py` importan el compartido);
  `42ab49a` = template con `octave` → `octave-chiloe` + `md-simulation`, `bench`, `mcp-blender-full`
  y `unrealMCP` (24 → 28).
- Config desplegada `~/.config/Claude/claude_desktop_config.json`: **22 servidores** — el `octave` viejo
  (`/home/astrid/octave-mcp/server.py`, 3 tools) **salió** y entró `md-simulation`
  (`mcp-ecosystem-chiloe/server.py`, 8 tools). Backup `claude_desktop_config.json.bak-20261109-202525`.
  El template quedó en **28 servidores**.
- **Paquetes de Octave visibles para toda la máquina** (fix C3-12, fuera del repo): registro en
  `/home/astrid/.config/octave/api-v61/octave_packages` (copia con las rutas reescritas hacia
  `~/.local/share/octave/api-v61/packages/`, con su `# length:` actualizado) y paquetes
  `control-4.2.3`, `signal-1.4.8`, `symbolic-3.2.2` en `~/.local/share/octave/api-v61/packages/`.
  El registro del runtime del workbench quedó intacto. `pkg install -local` de los paquetes de Debian
  **no** funciona (exige `COPYING`/`DESCRIPTION` con `Categories`; `package control is empty`).
- Claude Desktop 4.23118.0 (`claude-desktop-unofficial`) **no estaba corriendo**; relanzado con
  `XDG_CONFIG_HOME=/home/astrid/.config` y `XDG_DATA_HOME=/home/astrid/.local/share` →
  `--user-data-dir=/home/astrid/.config/Claude`. En `~/.config/Claude/logs/mcp.log` de la sesión
  nueva: **24 servidores** con handshake y `tools/list` OK; `openbb-mcp` cae por falta de red.
  **Trampa**: el shell del workbench exporta `XDG_CONFIG_HOME`/`XDG_DATA_HOME` al runtime de la app,
  así que lanzar Claude Desktop desde ahí lo hace leer un config **vacío** en el runtime.
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
  **Actualizado en este ciclo**: el `octave` viejo ya **salió** (duplicaba a `octave-chiloe`) y entró
  `md-simulation` → sigue en **22 servidores**, pero con una fuente de verdad para Octave
  (backup `claude_desktop_config.json.bak-20261109-202525`).
- Template: 28 → **24** claves (podadas `mcp-registry`, `inkscape`, `krita`,
  `materials-project` — binarios inexistentes — y **2 claves duplicadas** `analisis-suelos` y
  `bolsa-mcp`, cuya versión vieja pisaba a la nueva). Rutas `npx` corregidas al node real
  (`/home/astrid/.local/share/pi-node/node-v22.23.2-linux-x64/bin/npx`): `filesystem` pasó de
  caído a 14 tools y `firecrawl-mcp` de caído a 26 tools.
  **Actualizado en este ciclo**: 24 → **28** claves (`octave` → `octave-chiloe`, + `md-simulation`,
  `bench`, `mcp-blender-full`, `unrealMCP`); commit `42ab49a`.
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
- Ciclo 6 (2026-11-09): `mcp-ecosystem-chiloe` quedó en `3bf3141` (pusheado a `origin/main`), con tres
  commits: `065ed73` (versiona `formulas_tool.py` + `gazebo_utils.py` sin modificarlos y agrega
  `*.key/*.crt/*.pem/*.p12` al `.gitignore`), `8bfd6ba` (fixes propios: GROMACS en dos etapas + log real +
  COPASI 4.48 operativo + template con el venv) y `3bf3141` (el WIP del usuario commiteado byte a byte).
  `git ls-files` es la fuente de verdad para dependencias: `octave_mcp.py` importa
  `formulas_tool` y los 3 servidores gazebo (solo en el WIP) importan `gazebo_utils`; ambos
  módulos estaban **sin versionar** → el repo publicado fallaba con
  `ModuleNotFoundError: No module named 'formulas_tool'` en cualquier clon limpio. Tras el fix
  (módulos agregados sin modificar + `*.key/*.crt/*.pem/*.p12` en `.gitignore`): clon limpio
  importa 7/7 servidores y `octave_mcp` expone 13 tools.
- `shlex.split` **no** interpreta operadores de shell: con `shell=False`, un `&&` dentro del cmd
  llega como argumento literal. El WIP de `jobs.py` (shell=True→shell=False) rompía `run_gromacs`,
  que armaba `gmx grompp ... && gmx mdrun ...`. **Resuelto (`8bfd6ba`, opción A)**: `run_gromacs`
  corre `grompp` de forma síncrona (`subprocess.run` con argv list y timeout de 300 s, log en
  `cwd/grompp.log`; si falla devuelve `stage: "grompp"` y no lanza mdrun) y luego `mdrun` como job en
  background con `shlex.quote` por token. `run_lammps` no se ve afectado (su cmd no lleva operadores).
- `create_job` arma el id con `int(time.time())` → dos jobs del mismo motor+nombre en el mismo
  segundo **comparten id** y el segundo pisa al primero (`gromacs_mismo_1794247259`).
- GROMACS 2018+ escribe el log completo en el **directorio de trabajo** (`topol.log` / `md.log`), no en el
  stdout que captura el job: `parse_gromacs_log(log_path, cwd=None)` busca ahí (`GROMACS_LOG_NAMES`) y
  devuelve `log_file` con la ruta realmente leída. Además la prosa del programa contiene *"the error in
  the average pressure"*, así que detectar errores buscando `error` a secas da falsos positivos: el patrón
  correcto es `r"Fatal error|^ERROR \d+|\bSegmentation fault\b"` con `re.M`.
- En `mcp-ecosystem-chiloe` la config de `copasi` (desplegada y template) apunta a
  `/home/astrid/venv-copasi/bin/python` con `COPASI_BINDINGS_DIR=/home/astrid/venv-copasi/lib/python3.12/site-packages`:
  **COPASI 4.48.311 sí está instalado** en ese venv (el bloqueo original era el entorno de la config,
  no la instalación). `copasi_mcp.py` quedó adaptado a la API 4.48: `setReactionScheme` (no existe
  `setChemEq`), `saveModel()` escribe **CopasiML** (no SBML) → `_load_model()` prueba `loadModel()`
  antes que `importSBML()`, y al recargar un modelo se pierde el mapeo sustrato/producto → metabolito
  (la simulación aborta con `"Invalid state"` y serie en NaN) → `_reparar_mapeo_reacciones()` re-aplica
  esquema + función. Las concentraciones iniciales solo persisten con `setInitialExpression()` (tras
  `compileIfNecessary()`), no con `setInitialConcentration()`. La serie de salida viene en nº de
  partículas: hay que convertirla a concentraciones (÷ N_A ÷ volumen del compartimento) antes de
  calcular un SSE. Orden correcto: valor → reparar → compilar una vez → `applyInitialValues()` → simular.
- `mcp_suelos.ejecutar_octave` (WIP) cambia su contrato: devuelve JSON con `stdout`/`stderr`/
  `returncode` donde antes devolvía stdout crudo; los consumidores con texto plano deben
  actualizarse.
- Tool `MCP Inspector 2.13.0` (CLI): el **target va antes de las opciones** →
  `inspector --cli python3 /path/server.py --method tools/call --tool-name X --tool-arg k=v`;
  si el target va al final, `--tool-arg` se lo traga (`Invalid parameter format: python3`).
  `tools/list` devuelve `{"tools":[...]}`: parsear con `raw_decode` para contar tools.
