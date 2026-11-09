# Análisis vs. MCP real — `mcp-ecosystem-chiloe` (ciclo 5)

**Fecha:** 2026-11-09 · **Inspector:** `@modelcontextprotocol/inspector` 2.13.0
**Repo canónico analizado:** `/home/astrid/Documents/OpenScience/projects/githu/mcp-ecosystem-chiloe` (`480646d`, working tree con WIP sin commitear del usuario)
**Copia obsoleta presente en el sistema:** `/home/astrid/mcp-ecosystem-chiloe` (`c022adf`, 7 commits atrás)

---

## 0. Resumen ejecutivo

**Estado al cierre del ciclo 5 (post-corrección, re-verificado el 2026-11-09 14:26 con MCP Inspector 2.13.0):**

| Bloque | Servidores | Responden | Tools declaradas | Caídos | Antes → Después |
|---|---|---|---|---|---|
| `claude_desktop_config.template.json` | 24 | **21** | **250** | 3 (`gdal`, `openbb-mcp`, `qgis` — red/`uvx`) | 17/28 · 196 tools → **21/24 · 250 tools** |
| `~/.config/Claude/claude_desktop_config.json` (desplegada) | 22 | **21** | **734** | 1 (`openbb-mcp` — red/`uvx`) | 9/12 · 639 tools → **21/22 · 734 tools** |

**Diagnóstico original** (antes de corregir; el detalle está en las secciones 1–6):

| Bloque | Servidores | Responden | Tools declaradas | Servidores caídos |
|---|---|---|---|---|
| `claude_desktop_config.template.json` (28) | 28 | **17** | **196** | 11 |
| `~/.config/Claude/claude_desktop_config.json` (desplegada, 12) | 12 | **9** | **639** | 3 |

Tres capas de problemas, todas con evidencia ejecutada (no solo lectura de código):

1. **El `octave` que el usuario tiene desplegado no es el del repo.** El desplegado expone **3 tools**; el del repo expone **13**; la copia obsoleta del template expone **1**.
2. **19 de los 28 servidores del template no están en la config desplegada** — incluidos 11 servidores propios del repo (gazebo×3, brian2, copasi, neuron, sbtcvm, analisis-suelos, bolsa-mcp, solver…).
3. **Dos venvs con scipy roto** (`microbio-env`, `physics-env`): sus `.so` no son ELF → ambos servidores MCP mueren al arrancar, y aparecen en **ambas** configs.
4. **Dos bugs de código en el WIP sin commitear**: `verificar_mcp.py` quedó sin `import os` (→ `NameError` al arrancar) y `octave_mcp.py` responde `"id": null` en los errores (→ el cliente cuelga 60 s en vez de recibir el error).

> Las cuatro capas están **corregidas y verificadas**: ver §7 (cierre del ciclo). Las secciones 1–6 conservan el diagnóstico original tal como se midió, con dos correcciones señaladas explícitamente (A3 no era de severidad *alta*; A4 no era riesgo de timeout).

---

## 1. Método y reproducibilidad

```bash
cd /home/astrid/Documents/OpenScience/projects/githu
python3 analysis/mcp-ecosystem-chiloe/capture_tools.py
# -> analysis/mcp-ecosystem-chiloe/mcp_inspector_tools.json
```

- Herramienta real: MCP Inspector CLI (`inspector --cli --method tools/list <argv>`), **no** un mock.
- Timeout por servidor: 90 s. Ejecución **secuencial** (el Inspector en paralelo compite por su catálogo compartido y devuelve stdout vacío — reproducible).
- El Inspector devuelve `{"tools": [...]}` **sin** envoltura JSON-RPC → el parser debe aceptar esa forma.
- El Inspector **resuelve el target contra su cwd**: con cwd = directorio del script + ruta absoluta duplica el prefijo y el server no arranca. Workaround en el script: cwd neutral (`/tmp`) + rutas absolutas.
- Rutas de la copia obsoleta en el template se remapean a la canónica y se marca cada remapeo en el campo `nota` de la evidencia.

Evidencia completa (argv, cantidad de tools, segundos, stderr): `mcp_inspector_tools.json`.

---

## 2. Evidencia por servidor

### 2.1 Template (`claude_desktop_config.template.json`) — 17/28 responden, 196 tools

| Servidor | Estado | Tools | s | Observación |
|---|---|---|---|---|
| analisis-suelos | ok | 2 | 1.6 | ruta remapeada a canónica |
| biosim-mcp | ok | 6 | 2.6 | |
| blender | ok | 22 | 2.3 | |
| bolsa-mcp | ok | 1 | 2.0 | WIP: import corregido |
| brian2-mcp | ok | 9 | 3.0 | |
| copasi | ok | 8 | 1.3 | |
| fetch | ok | 1 | 2.3 | |
| **filesystem** | **caído** | 0 | 0.5 | nvm no instalado (`/home/astrid/.nvm/versions/node/` no existe) |
| **firecrawl-mcp** | **caído** | 0 | 0.4 | idem (npx v22.22.3 inexistente) |
| freecad | ok | 17 | 2.4 | versión distinta a la desplegada (59) |
| gazebo-core | ok | 16 | 1.3 | |
| gazebo-entities | ok | 9 | 1.4 | |
| gazebo-quantum | ok | 4 | 4.5 | |
| **gdal** | **caído** | 0 | 17.4 | timeout: `uvx --from gdal-mcp` necesita red |
| github | ok | 44 | 0.5 | binario Go, no Python |
| **inkscape** | **caído** | 0 | 0.5 | venv `/home/astrid/.config/inkscape/extensions/inkmcp/inkmcp/venv` no existe |
| **krita** | **caído** | 0 | 0.5 | `/home/astrid/krita-mcp/server.py` no existe |
| **materials-project** | **caído** | 0 | 0.4 | `/home/astrid/.venv/mcp/bin/mcp-materials-project` no existe |
| **mcp-registry** | **caído** | 0 | 0.5 | `/home/astrid/.local/bin/mcp-registry` no existe |
| **microbio-mcp** | **caído** | 0 | 1.2 | scipy roto (ver B2) |
| neuron-mcp | ok | 9 | 1.4 | |
| octave | ok | **13** | 1.0 | ruta remapeada a la canónica |
| **openbb-mcp** | **caído** | 0 | 17.4 | timeout: `uvx --from openbb-mcp-server` |
| **physics-mcp** | **caído** | 0 | 1.2 | scipy roto (ver B2) |
| psyche-mcp | ok | 10 | 2.8 | |
| **qgis** | **caído** | 0 | 1.7 | `uvx --from git+https://…` necesita red |
| sbtcvm | ok | 10 | 1.3 | |
| solvespace | ok | 15 | 0.7 | |

### 2.2 Config desplegada (12) — 9/12 responden, 639 tools

| Servidor | Estado | Tools | s | Observación |
|---|---|---|---|---|
| bench | ok | 7 | 0.5 | no está en el template |
| biosim-mcp | ok | 6 | 2.4 | |
| blender | ok | 22 | 2.3 | |
| freecad | ok | **59** | 1.3 | ≠ 17 del template: es otro servidor |
| github | ok | 44 | 0.5 | |
| mcp-blender-full | ok | **218** | 1.3 | no está en el template |
| **microbio-mcp** | **caído** | 0 | 1.3 | scipy roto (ver B2) |
| **octave** | ok | **3** | 3.7 | es `/home/astrid/octave-mcp/server.py` (214 módulos, **3 tools expuestas**) |
| **openbb-mcp** | **caído** | 0 | 17.5 | timeout uvx |
| **physics-mcp** | **caído** | 0 | 1.3 | scipy roto (ver B2) |
| psyche-mcp | ok | 10 | 2.8 | |
| unrealMCP | ok | **270** | 1.9 | no está en el template |

### 2.3 Cobertura template ↔ desplegada

- **En el template y NO desplegados (19):** analisis-suelos, bolsa-mcp, brian2-mcp, copasi, fetch, filesystem, firecrawl-mcp, gazebo-core, gazebo-entities, gazebo-quantum, gdal, inkscape, krita, materials-project, mcp-registry, neuron-mcp, qgis, sbtcvm, solvespace.
- **Desplegados y NO en el template (3):** bench, mcp-blender-full, unrealMCP.
- **En ambos (9):** biosim-mcp, blender, freecad, github, microbio-mcp, octave, openbb-mcp, physics-mcp, psyche-mcp.
- De los 9 comunes, **3 están caídos en ambas** (microbio, openbb, physics) y **1 expone un número distinto de tools** (freecad 59 vs 17; octave 3 vs 13).

---

## 3. Hallazgos

Severidad: **crítico** = deja el servidor sin funcionar; **alto** = functionality perdida o bug reproducible; **medio** = fragilidad / deuda; **bajo** = cosmético.

### A. Bugs de código (repo canónico, working tree)

**A1 — crítico · `verificar_mcp.py:59` · el WIP del usuario reintrodujo un `NameError`**

```
59:    env = os.environ.copy()
```

En `HEAD` el archivo importa `os` (línea 16); en el **working tree** ese `import os` fue eliminado y el uso permanece → `NameError: name 'os' is not defined` al arrancar el verificador, antes de tocar ningún servidor. El resto del WIP (firmas tipadas, +88 líneas) es una mejora; el único defecto es la Import quitada de más.

**A2 — alto · `servidores-propios/octave_mcp.py:210` · errores sin `id` correlacionable**

El manejador de excepciones responde `"id": None` en lugar de propagar el `id` de la petición. El cliente no puede correlacionar la respuesta → en vez de recibir el error cuelga hasta el timeout (medido: 60 s con MCP Inspector) y el usuario ve "Connection closed" genérico. Es el **único** archivo de `servidores-propios/` con ese patrón (grep `"id": None`), por lo que el bug es local y de corrección puntual.

**A3 — medio (no *alto*, ver nota) · `middle_square_weyl_tool.py:67` y `ancestral_octave_tool.py:533` · import frágil y ambiguo**

```python
from server import run_octave
```

`run_octave` **no** está en ningún `server.py` del repo: el `server.py` de la raíz es el servidor *md-simulation* (8.229 bytes, sin `run_octave`). La función real vive en `servidores-propios/octave_mcp.py`, así que el import correcto sería `from octave_mcp import run_octave` (mismo directorio). Verificado:

- Import standalone del módulo tool → `ModuleNotFoundError: No module named 'server'`.
- Hay **8 copias** de `octave_mcp.py` con `run_octave` en el sistema (`/home/astrid/octave-mcp/`, `/home/astrid/github/octave-mcp/`, `/home/astrid/github/mcp-ecosystem-chiloe/`, `Open-Science/notebooks/…`, …) → la resolución en runtime depende del `sys.path`, no del repo.

> **Corrección del diagnóstico inicial:** la severidad *alta* era incorrecta. En `HEAD`, `octave_mcp.py` ya inserta `sys.modules.setdefault("server", sys.modules["__main__"])`, de modo que el caso *in-server* (que es el que usa Claude Desktop) **funcionaba**; lo que fallaba era el **uso standalone** del módulo tool (import directo) y la resolución ambigua entre 8 copias. El arreglo (§7) lo resuelve por ambos lados.

**A4 — medio · `middle_square_weyl_tool.py` · `modulus` sin validar**

Con `modulus=1000` el script Octave desborda: `seen(1025): out of bound 1000 (dimensions are 1x1000)`, porque `ms_next` devuelve valores hasta 9999. El default (10000) no desborda, pero un `modulus` chico produce un error críptico de Octave en vez de una validación en el borde.

> **Corrección del diagnóstico inicial:** la preocupación "al borde del timeout" era infundada. Medido: con los defaults la corrida completa tarda **17.4 s** (timeout interno 30 s) y `max_iter` es irrelevante porque los bucles cortan al primer ciclo (periodos 90 y 111). El riesgo real era solo el `modulus` chico → validado en el borde (§7).

**A5 — ya corregido en el WIP · `octave_mcp.py` · binario de Octave por nombre**

El HEAD invocaba `["octave", …]` (depende del `PATH` del cliente). El WIP agrega `shlex`/`shutil` + `_octave_bin()` y llama `[_octave_bin(), "--no-gui", "--eval", code]` → correcto.

**A6 — ya corregido en el WIP · `bolsa_mcp.py` · import de FastMCP**

`from fastmcp import FastMCP` (paquete inexistente) → `from mcp.server.fastmcp import FastMCP` + nombre del server `bolsa-mcp`. El WIP además renombra el archivo a `bolsa_mcp.py`.

### B. Entorno / despliegue

**B1 — crítico · `octave` desplegado ≠ `octave` del repo**

El desplegado (`/home/astrid/octave-mcp/server.py`) **solo arranca con su propio venv**:

| Intérprete | Resultado medido |
|---|---|
| `python3` (sistema) | `ModuleNotFoundError: No module named 'ocas'` (`server.py:92` → `ocas_symbolic_tool.py:24`) — muere al arrancar |
| `/home/astrid/octave-mcp/.venv/bin/python` | handshake OK en **5.6 s** |

Claude Desktop (`claude-desktop-unofficial`, PID 15869) lo lanza con el venv → **funciona**, pero es el server viejo de otro repo con **3 tools** (el del repo expone **13**; la copia obsoleta, **1**). El WIP del template ya apunta al `octave_mcp.py` del repo — pero a la **copia obsoleta** (`/home/astrid/mcp-ecosystem-chiloe/…`, 7 commits atrás, 1 tool) → el arreglo apunta al lugar equivocado.

**B2 — alto · scipy roto en `microbio-env` y `physics-env`**

```
ImportError: …/scipy/_lib/_uarray/_uarray.cpython-312-x86_64-linux-gnu.so: invalid ELF header
ImportError: …/scipy/spatial/_distance_wrap.cpython-312-x86_64-linux-gnu.so: invalid ELF header
```

Los `.so` existen (115 KB / 120 KB) pero **no son ELF** (`file` → `data`; magic `00 10 dd fc` / `00 c0 29 ff`) → wheels de otra plataforma o instalación corrompida. Ambos servidores caen al importar scipy, y ambos están en **las dos** configs.

**B3 — alto · 19 servidores del template ausentes en lo desplegado**

Incluye los 11 propios del repo. El template describe un ecosistema que el usuario no tiene disponible en Claude Desktop.

**B4 — alto · doble copia divergente del repo**

`/home/astrid/mcp-ecosystem-chiloe` (`c022adf`) vs canónica en `githu` (`480646d`). La copia vieja no tiene los `TOOL_SCHEMA` nuevos y su `octave_mcp.py` expone 1 tool en vez de 13. El template apunta a la vieja → **lo que se despliega no tiene las tools nuevas** (goertzel, ancestral_octave, middle_square_weyl).

**B5 — medio · rutas del template que no existen en la máquina**

`~/.nvm/versions/node/` (nvm no instalado) → `filesystem` y `firecrawl-mcp`; `/home/astrid/krita-mcp/server.py`; `/home/astrid/.venv/mcp/bin/mcp-materials-project`; `/home/astrid/.local/bin/mcp-registry`; venv de inkscape.

**B6 — medio · verificación no determinista offline**

`gdal`, `openbb-mcp` y `qgis` son `uvx` que resuelven por red → 17 s y timeout de 90 s en la captura. No son bugs del repo, pero hacen que "el servidor está caído" no sea una afirmación sólida sin red.

**B7 — bajo · mismo nombre, distinto servidor**

`freecad` del template = 17 tools vs 59 de la desplegada → son binarios distintos con el mismo nombre en la config.

### C. Higiene del repo

**C1** · `py_compile` sobre los 31 `.py` trackeados: sin errores. `ruff`: 28 hallazgos (15 F401 imports sin usar, 9 E402 import fuera de módulo, 1 E401, 1 E702, 1 E741 `l` ambiguo en `ancestral_octave_tool.py:541`, 1 F821 = el `os` de A1).
**C2** · Los 8 módulos tool del repo exponen esquemas **coherentes** con sus firmas (`lyapunov_tool`, `stiff_ode_tool`, `bifurcation_tool`, `hilbert_tool`, `formulas_tool`, `goertzel_tool`, `ancestral_octave_tool`, `middle_square_weyl_tool`): la correspondencia schema↔firma verificada sin discrepancias.
**C3** · El working tree del usuario tiene 12 archivos modificados (incluidos `jobs.py`, `copasi_mcp.py`, `gazebo_*.py`, `mcp_suelos.py`, `hilbert_tool.py`, `btfloat.py`) que **este informe no auditó**: quedan fuera del alcance del ciclo 5.

---

## 4. Qué resuelve el WIP del usuario (y qué falta)

| Ítem | Estado en el WIP | Pendiente |
|---|---|---|
| `verificar_mcp.py` sin `import os` | **introduce** el bug (A1) | reponer `import os` |
| `octave` → repo | ✅ pero apunta a la **copia obsoleta** (B4) | apuntar a la canónica |
| `bolsa_mcp` import FastMCP | ✅ | — |
| `octave_mcp` binario por `PATH` | ✅ (`_octave_bin()`) | — |
| `id: null` en errores (A2) | ❌ no tocado | propagar `req_id` |
| `from server import run_octave` (A3) | ❌ no tocado | `from octave_mcp import run_octave` |
| scipy roto (B2) | ❌ (entorno) | reinstalar scipy en ambos venvs |
| rutas inexistentes (B5) | ❌ | podar o instalar |
| 19 servidores no desplegados (B3) | ❌ | sincronizar template ↔ config real |

---

## 5. Recomendaciones (orden de ejecución sugerido)

1. **P0 — una línea:** `import os` en `verificar_mcp.py` (HEAD lo tenía; el WIP lo borró).
2. **P0 — una línea:** en `octave_mcp.py:210`, responder con el `id` de la petición en vez de `None`.
3. **P0 — entorno:** `pip install --force-reinstall scipy` en `/home/astrid/microbio-env` y `/home/astrid/physics-env` (los `.so` no son ELF).
4. **P1 — rutas:** en `claude_desktop_config.template.json`, apuntar `octave` a la copia canónica; podar las entradas sin binario (filesystem/firecrawl/krita/materials-project/mcp-registry/inkscape) o instalar lo que falte.
5. **P1 — imports:** `from octave_mcp import run_octave` en `middle_square_weyl_tool.py:67` y `ancestral_octave_tool.py:533`.
6. **P1 — cobertura:** decidir si la config desplegada debe incorporar los 11 servidores propios del repo (hoy 0 de 11 disponibles) y si `bench`, `mcp-blender-full` y `unrealMCP` deben entrar al template.
7. **P2 — robustez:** validar `modulus >= 10000` en `middle_square_weyl` y revisar el timeout de 30 s de `run_octave` frente al costo real del default.
8. **P2 — higiene:** `ruff --fix` sobre F401/E402 y renombrar la variable `l` en `ancestral_octave_tool.py:541`.

---

## 6. Limitaciones de esta verificación

- No se auditó el WIP de 12 archivos (C3) ni el contenido funcional de las 196/639 tools: solo su **existencia y conteo** vía `tools/list`.
- Los servidores `uvx` (gdal, openbb, qgis) no se pudieron distinguir entre "caído" y "sin red" en esta corrida.
- No se ejecutó el ciclo completo de `verificar_mcp.py` (A1 lo impide tal como está el working tree).
- La clasificación "caído" es de esta máquina y esta fecha; `n_tools` refleja el estado del `tools/list` de hoy.

---

## 7. Cierre del ciclo: correcciones aplicadas y verificadas

Todas las correcciones se midieron con **MCP Inspector 2.13.0 real** (`tools/list` y `tools/call`), nunca por lectura de código.

### 7.1 Correcciones de código (repo canónico)

| Ítem | Corrección | Evidencia |
|---|---|---|
| **A1** | `verificar_mcp.py`: repuesto `import os` (el WIP lo había borrado y el uso seguía) | `py_compile` OK; el archivo vuelve al estado de `HEAD` en ese punto → el fix **no genera diff** |
| **A2** | `octave_mcp.py`: `req_id = None` antes del `try`; el `except` responde `"id": req_id` | `tools/call middle_square_weyl --tool-arg modulus=1000` → error devuelto en **1.0 s** con `id` correlacionable (antes: cuelgue 60 s) |
| **A3** | Nuevo `servidores-propios/octave_runner.py` (`_octave_bin()` + `run_octave(code, timeout=30)`); `octave_mcp.py`, `middle_square_weyl_tool.py` y `ancestral_octave_tool.py` importan desde ahí (fin del shim `"server"` y del acoplamiento a 8 copias) | import **standalone** del módulo tool OK; `octave_mcp.py` expone **13 tools**; `tools/list` del server desplegado = 13 |
| **A4** | `middle_square_weyl_tool.py`: validaciones en el borde (`digits >= 1`, `digits` par, `modulus >= 10**digits`, `max_iter >= 1`) + `digits` **funcional** (antes ignorado) | `digits=3` → error claro; `digits=2, modulus=100` → 5 ciclos / 0.62 / weyl 6 / tcr 7; defaults → **baseline reproducido exactamente** (ciclos 8, `zero_sink` 0.1818, `weyl_period` 90, `tcr_period` 111, `tcr_modulus_M` 99400900.0, `variant_spread` 0.189189; `MATCH BASELINE` = True) |

Higiene asociada: `ruff` sobre los 5 archivos tocados → **"All checks passed!"** (13 → 0: F821 `os`, F401 `re`/`shlex`, E401, E741 `l`→`line`); `py_compile` OK en los 5.

### 7.2 Correcciones de entorno y despliegue

| Ítem | Corrección | Evidencia |
|---|---|---|
| **B2** | `pip install --force-reinstall --only-binary=:all: scipy` en `/home/astrid/microbio-env` y `/home/astrid/physics-env` (los `.so` no eran ELF: magic `00 10 dd fc`) | scipy **1.18.1** importa en ambos; `microbio-mcp` = 6 tools y `physics-mcp` = 8 tools → **ambos vuelven a responder** en template y desplegada |
| **B1/B4** | Template: `octave` y los 9 servidores propios apuntando a la ruta **canónica** (`/home/astrid/Documents/OpenScience/projects/githu/mcp-ecosystem-chiloe/servidores-propios/…`), no a la copia obsoleta | `octave` = **13 tools** en el template (la copia obsoleta exponía 1) |
| **B3** | Config desplegada: **backup** `claude_desktop_config.json.bak-20261109-142058` + se agregan 10 entradas (9 servidores propios + `octave-chiloe`), **sin tocar** las 12 existentes | las 10 verificadas una por una: 13 + 2 + 1 + 10 + 9 + 9 + 8 + 16 + 9 + 4 = **81 tools**; desplegada: 12 → 22 servidores, 639 → **734 tools** |
| **B5** | Podadas 4 entradas sin instalación (`mcp-registry`, `inkscape`, `krita`, `materials-project`) + **eliminadas 2 claves duplicadas** (`analisis-suelos`, `bolsa-mcp`: la copia vieja pisaba a la nueva) + rutas `npx` inexistentes reemplazadas por el node real (`/home/astrid/.local/share/pi-node/node-v22.23.2-linux-x64/bin/npx`) | template: 28 → **24** claves, **0 claves duplicadas**, **0 rutas absolutas inexistentes**; `filesystem` = **14 tools** (antes caído) y `firecrawl-mcp` = **26 tools** (antes caído) |

### 7.3 Evidencia final (2026-11-09 14:26)

**Template (24 servidores): 21/21 respondiendo salvo 3 por red** — `gdal`, `openbb-mcp`, `qgis` (todos `uvx` que resuelven por red; no son bugs del repo).

| Caído | Tools | s | Causa |
|---|---|---|---|
| gdal | 0 | 17.4 | `uvx --from gdal-mcp` → timeout de red |
| openbb-mcp | 0 | 17.6 | `uvx --from openbb-mcp-server` → timeout de red |
| qgis | 0 | 1.7 | `uvx --from git+https://github.com/nkarasiak/qgis-mcp` → timeout de red |

**Desplegada (22 servidores): 21/22 responden** — único caído: `openbb-mcp` (misma causa de red).

### 7.4 Qué queda pendiente (decisiones del usuario, no bloquean el ciclo)

- **`octave` desplegado sigue siendo el server de otro repo** (3 tools genéricas: `list_tools`/`get_tool_schema`/`call_tool`, `/home/astrid/octave-mcp/server.py`). Se **conservó** a propósito y el server del repo se agregó aparte como `octave-chiloe` (13 tools). Si el usuario quiere reemplazar el viejo, es borrar una entrada.
- **Los 3 servidores `uvx`** no se pueden verificar offline: requieren red o un install previo.
- **`bench`, `mcp-blender-full` y `unrealMCP`** existen en la config desplegada y **no** están en el template: el template sigue siendo la fuente de los servidores propios del repo, no un espejo exacto del entorno.
- El **working tree del repo canónico** conserva WIP del usuario sin commitear (incluido el template y `verificar_mcp.py`): los commits de este ciclo incluyen **solo** los 4 archivos de código propios (`octave_runner.py` nuevo, `octave_mcp.py`, `middle_square_weyl_tool.py`, `ancestral_octave_tool.py`).
