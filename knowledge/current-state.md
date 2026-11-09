# Current State

- Goal: entregar análisis estáticos de repos vía VS Code MCP + MCP Inspector.
- Stage: ciclo 3 (Ternario-ancestral-sensores) completado y verificado
  (informe en `analysis/ternario-ancestral-sensores/ANALISIS_VS_MCP.md`).
- Current task: informe entregado. Cobertura: kernel bare-metal completo (45 `.c`, 20 413
  líneas), userspace TAK (29 `.c`), 47 `.py`, 10 `.m` (Octave), 5 `.sh`, 2 `.ino`. Instrumentos:
  VS Code MCP (Pylance + Ruff + cpptools) vía MCP Inspector 2.13.0, gcc 13.3.0 (`-m32`),
  ruff CLI, Octave y ejecución real. 12 bugs reales confirmados (Python: 2 NameError en la
  GUI, 1 SyntaxError, 1 sombreado de paquete; C: FAT16 0xE5 con char con signo, OOB de 1 byte
  en USB, `getchar` sin inicializar, 41 buffers de 2 bytes con `num_to_str`, 5 declaraciones
  implícitas, 3 guardas de rango inoperantes; científico: mezcla de modelos de Landauer).
  El kernel y el userspace compilan y linkean; los 2 experimentos Octave corren.
- Next action: decidir con el usuario si se aplican las correcciones priorizadas del informe
  (`tritos_gui.py` primero: `import sys` + `disk_img`; después `tritos_lab/` y el marcador
  FAT16), o si se continúa con otro repo.

## Facts_changed

- `Ternario-ancestral-sensores` está clonado en `/home/astrid/Ternario-ancestral-sensores`
  (`master` @ `f91a669`); es el OS "Tritos" y tiene su propio `AGENTS.md`/`KNOWLEDGE.md`/
  `notes/`. Análisis de solo lectura (único archivo escrito: probe de control, eliminado).
- Bugs de Tritos verificados: `tritos_gui.py` usa `sys` sin importarlo (7 usos) y `disk.img`
  en vez de `disk_img` (2388); `tritos_lab/tritos_godot.py:98` tiene un SyntaxError;
  `tritos_lab.py` sombrea al paquete `tritos_lab/` (ImportError en Blender y Godot);
  `fs.c` compara 0xE5 contra `char` con signo (6 sitios → el borrado FAT16 es inoperante);
  `usb_storage.c:86` escribe `product[16]` en `char product[16]` (corrompe `mounted`);
  `libc_full.c:345` devuelve `char` sin inicializar; `num_to_str` (`ternary.h:273`) no acota
  el destino y hay 41 llamadas con `char nb[2]`; `elf_dynamic.c:133` y `scheduler.c:123,128`
  tienen guardas que nunca se disparan (contador `uint8_t` vs límites 256/260/365);
  `run_all.m:88-94` mezcla dos modelos de coste de Landauer (imprime 1.585 y "5.7 % más
  eficiente" a la vez).
- `build-kernel.sh` está obsoleto frente al `Makefile`: compila 3 de 45 TUs y su ISO usa
  `linux16` para una imagen multiboot; el `Makefile` construye los 45 y usa `multiboot`.
- En `Ternario-ancestral-sensores` el barrido con gcc da 89 avisos en 27 archivos del kernel
  (9 `type-limits`, 5 `implicit-function-declaration`, 2 `uninitialized`, 2 `maybe-uninitialized`,
  2 `pointer-to-int-cast`); los `.py` de la raíz dan 51 hallazgos ruff y los del
  `experiment-suite` 22.


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
