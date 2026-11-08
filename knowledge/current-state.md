# Current State

- Goal: entregar el análisis estático de `codigos-potenciados` mediante las herramientas MCP de VS Code.
- Stage: completado y verificado.
- Current task: informe en `codigos-potenciados/ANALISIS_VS_MCP.md`; 25 archivos C/H analizados
  (8 hints MCP: 6 includes removibles, 2 falsos positivos probados por recompilación; warnings de
  gcc con `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual`;
  7 estilos de cppcheck; 0 errores).
- Next action: decidir con el usuario si se aplican los cambios recomendados (quitar 5 includes
  removibles, declarar 4 prototypes faltantes, corregir conversiones y el `cast` que descarta `const`).

## Facts_changed

- El puente VS Code MCP requiere que VS Code se lance con `XDG_DATA_HOME=/home/astrid/.local/share`
  (igual que el servidor MCP en `~/.config/opencode/opencode.json`); con el `XDG_DATA_HOME` largo del
  workbench el path del socket Unix supera los 107 caracteres y el bind falla.
- `codigos-potenciados` no es un repo git propio: es un subdirectorio del repo `githu`
  (raíz: `/home/astrid/Documents/OpenScience/projects/githu`).
- VS Code tiene ahora `ms-vscode.cpptools` v1.35.6 instalado; el análisis C/C++ real requiere
  además `.vscode/c_cpp_properties.json` + `compile_commands.json` generado por
  `codigos-potenciados/tools/gen_compile_commands.py`.
- En Linux cpptools no propaga warnings del compilador al panel de Problemas (solo semántica
  IntelliSense); los warnings de gcc hay que obtenerlos aparte.
