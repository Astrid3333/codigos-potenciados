# Current State

- Goal: entregar el análisis estático de `octave-mcp` mediante las herramientas MCP de VS Code.
- Stage: completado y verificado (informe en `analysis/octave-mcp/ANALISIS_VS_MCP.md`).
- Current task: informe entregado. Cobertura: los 19 archivos del manifiesto estricto de CI
  (`ci/ruff_files.txt`) + 13 archivos legacy seleccionados por gravedad de lint, con dos
  analizadores independientes dentro de VS Code (Pylance 2026.4.1 y extensión Ruff) más
  contraste con `ruff` 0.15.19 en CLI y ejecución directa de módulos. 109 diagnósticos en el
  manifiesto estricto (88 ERROR, 21 HINT/WARN): ~60 por contrato de tipos sin anotar y ~20 por
  stubs de sympy/scipy; 5 bugs reales confirmados (2 verificados por ejecución).
- Next action: decidir con el usuario si se aplican las correcciones recomendadas (priorizadas
  en el informe; el arreglo de `plotting_tools.py` en primer lugar, más una decisión de
  arquitectura sobre el subpaquete `tools/`), o si se continúa con otro repo.

## Facts_changed

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
