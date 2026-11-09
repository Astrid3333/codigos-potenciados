# Auditoría del WIP del usuario — `mcp-ecosystem-chiloe`

**Fecha:** 2026-11-09 · **Ciclo:** 6 (continuación del ciclo 5) · **Base auditada:** `982ab0f`
**Alcance:** los 11 archivos modificados sin commitear + los 12 untracked del repo canónico
`/home/astrid/Documents/OpenScience/projects/githu/mcp-ecosystem-chiloe`
**Método:** diff vs HEAD · ruff con atribución preexistente/introducido · grafo de imports vs `git ls-files` ·
clon limpio de `origin/main` · MCP Inspector 2.13.0 (`tools/list` + `tools/call`, secuencial, cwd `/tmp`) ·
A/B del pipeline GROMACS por MCP contra HEAD y contra el WIP.
**Estado final:** `065ed73` (fix de versionado) → `8bfd6ba` (fixes propios) → `3bf3141` (WIP commiteado tal cual),
los tres pusheados a `origin/main`.

Evidencia cruda: `wip_audit_evidence.json`.

---

## 0. Veredicto

El WIP **mejora** de forma real la robustez del código (timeouts, temporales seguros, fin de `shell=True`,
type hints, errores explícitos), pero trae **dos regresiones y una dependencia rota**:

| # | Severidad | Hallazgo | Estado |
|---|-----------|----------|--------|
| C3-1 | **alta** | `formulas_tool.py` sin versionar → el repo publicado no arranca en un clon limpio | **Corregido** (`065ed73`) |
| C3-2 | **alta** | `jobs.py` con `shell=False` + `shlex.split` **rompe `run_gromacs`** (el cmd lleva `&&`) | **Corregido** (`8bfd6ba`, opción A) |
| C3-3 | media | `gazebo_utils.py` sin versionar → el WIP de los 3 servidores gazebo no es reproducible | **Corregido** (`065ed73`) |
| C3-4 | media | COPASI: 9 tools expuestas pero el módulo no está instalado | **Corregido** (`8bfd6ba`) |
| C3-5 | baja | Colisión de `job_id` (mismo motor+nombre en el mismo segundo) | Abierto (WIP commiteado tal cual) |
| C3-6 | baja | 6 imports sin usar nuevos (3 en modificados, 3 en el archivo nuevo) | Abierto (WIP commiteado tal cual) |
| C3-7 | baja | `_octave_bin()` duplicado en 3 archivos | Abierto (WIP commiteado tal cual) |
| C3-8 | info | `mcp_suelos.ejecutar_octave` cambia su contrato (JSON en vez de stdout crudo) | A documentar |
| C3-9 | info | Clave TLS privada sin trackear **y sin regla de ignore** | **Corregido** (`065ed73`) |
| C3-10 | media | `parse_gromacs_log` leía el stdout del job; el log real de GROMACS 2018+ está en `topol.log` → `finished` y los bloques de energía nunca aparecían | **Corregido** (`8bfd6ba`) |
| C3-11 | media | El detector de errores buscaba `error` a secas → la prosa del programa ("the error in the average pressure") disparaba un falso positivo en toda simulación exitosa | **Corregido** (`8bfd6ba`) |

Además, el WIP es **mejor que HEAD en 10 puntos verificados** (§3), incluido el fix real de
inyección de shell en `jobs.py` — que es justo lo que causa C3-2.

**Estado de commits en `mcp-ecosystem-chiloe` (branch `main`, `origin/main`):**

| commit | contenido |
|---|---|
| `065ed73` | versiona `formulas_tool.py` y `gazebo_utils.py` sin modificarlos; `.gitignore` para claves/certs |
| `8bfd6ba` | fixes propios: GROMACS en dos etapas + log real + COPASI 4.48.311 operativo + template con el venv |
| `3bf3141` | tu WIP commiteado byte a byte (`jobs.py`, `verificar_mcp.py` y 7 servidores-propios) |

---

## 1. C3-2 — `shell=False` rompe GROMACS (diagnóstico + resolución)

`server.py:140` arma el comando de GROMACS con un `&&`:

```python
cmd = (
    f"gmx grompp -f {mdp_path} -c {gro_path} -p {top_path} -o topol.tpr -maxwarn 5 "
    f"&& gmx mdrun -deffnm topol {nt_flag}"
)
```

El WIP de `jobs.py` cambió `shell=True` por `shell=False` + `shlex.split(job["cmd"])`.
`shlex.split` **no** interpreta operadores de shell: el `&&` pasa a ser un argumento literal.

**Nivel Python (mismo `jobs.py`, HEAD vs WIP, comando `echo uno && echo dos`):**

| variante | salida del log |
|---|---|
| HEAD (`shell=True`) | `uno` / `dos` (dos líneas) |
| WIP (`shell=False`) | `uno && echo dos` (una línea) |

**Nivel MCP, GROMACS 2023.3 real (sistema de un átomo, `nsteps=0`), vía `run_gromacs`:**

```
WIP  → gmx grompp ... -maxwarn 5 && gmx mdrun -deffnm topol
       Invalid command-line options
         In command-line option -maxwarn
           Invalid value: '&&'; expected an integer
           Invalid value: 'gmx'; expected an integer
           Invalid value: 'mdrun'; expected an integer
           Unknown command-line option -deffnm
       → no se ejecuta nada

HEAD → gmx grompp ... -o topol.tpr -maxwarn 5 && gmx mdrun -deffnm topol
       starting mdrun 'Argon one-atom test'
       0 steps,      0.0 ps.
       Performance:        8.199        2.927
       → pipeline completo
```

**No toqué tu WIP** en esto: es tu decisión de diseño. Las dos salidas limpias son:

- **A. Mantener `shell=False`** (seguro) y partir el pipeline GROMACS en dos pasos dentro de `run_gromacs`
  (`grompp` síncrono → `mdrun` como job). `run_lammps` no cambia: su cmd no lleva operadores.
- **B. Mantener `shell=True`** y construir el cmd con `shlex.quote()` por token en vez de interpolar rutas crudas.
  El riesgo de inyección queda si `mdp_path`/`gro_path`/`top_path` llevan metacaracteres.

Decime cuál y lo aplico.

### Resolución aplicada (`8bfd6ba`) — opción A

Eligiste A y quedó implementado y verificado por MCP:

```python
# server.py:run_gromacs
grompp_argv = [gmx_bin, "grompp", "-f", mdp, "-c", gro, "-p", top, "-o", "topol.tpr", "-maxwarn", "5"]
# etapa 1, SINCRÓNICA: subprocess.run(argv_list, cwd=cwd, timeout=300)
#   si returncode != 0 -> {"error": "grompp fallo; no se lanzo la simulacion",
#                         "stage": "grompp", "returncode": 1, "log_path": ..., "log_tail": ...}
#   y NO se lanza mdrun
# etapa 2, BACKGROUND: mdrun_argv = [gmx_bin, "mdrun", "-deffnm", "topol", "-nt", n]
#   cmd = " ".join(shlex.quote(tok) for tok in mdrun_argv)   -> jobs.create_job / start_job
```

El `shell=False` de tu WIP se conserva (es la parte que hay que mantener); lo que se elimina es el `&&`,
que era el único motivo por el que no podía seguir usándose.

**Verificación por MCP (GROMACS 2023.3 real):**

| caso | resultado |
|---|---|
| camino feliz (job `gromacs_run_fix_1794250922`) | `Invalid value: '&&'` → **0 ocurrencias**; `parse_results` → `finished: true`, `energy_blocks_found: 3` |
| camino de error (grompp inválido) | `{"error": "grompp fallo; no se lanzo la simulacion", "stage": "grompp", "returncode": 1}` — mdrun nunca se lanza |
| run largo (200 pasos, job `gromacs_run_long_1794256195`) | `finished: true`, `had_error_keyword: false`, `performance: "299.413 ns/day"`, `log_file: /tmp/opencode/mdtest/run_long/topol.log` |

### C3-10 / C3-11 — dos bugs de `parsers.py` que el A/B destapó

No estaban en tu WIP (venían de HEAD), pero con el pipeline arreglado se volvieron visibles porque el log
dejaba de "fallar" por otra razón:

1. **El log no estaba donde se lo buscaba.** GROMACS 2018+ escribe el log completo en el directorio de
   trabajo (`topol.log` / `md.log`), no en el stdout que captura el job. `parse_gromacs_log` leía solo el
   stdout → `finished: false` y `energy_blocks_found: 0` en simulaciones que sí habían terminado.
   Ahora `parse_gromacs_log(log_path, cwd=None)` busca primero en el cwd (`GROMACS_LOG_NAMES`) y devuelve
   `log_file` con la ruta real usada; `server.py` le pasa `job.get("cwd")`.
2. **El detector de errores era una aguja en un pajar.** `re.search(r"Fatal error|error", text, re.I)`
   matcheaba *"the error in the average pressure"*, que GROMACS escribe en cada bloque de energía
   → `had_error_keyword: true` en toda simulación exitosa. Ahora el patrón es
   `r"Fatal error|^ERROR \d+|\bSegmentation fault\b"` con `re.M`.

---

## 2. C3-1 / C3-3 — el repo publicado estaba roto

`octave_mcp.py:12` importa `formulas_tool`, y el WIP de los tres servidores Gazebo importa `gazebo_utils`.
**Ninguno de los dos estaba versionado.** Clon limpio de `origin/main` (`982ab0f`):

```
File ".../servidores-propios/octave_mcp.py", line 12, in <module>
    from formulas_tool import (
ModuleNotFoundError: No module named 'formulas_tool'
```

Es decir: el repo que publiqué en el ciclo 5 no arrancaba para nadie más, en el servidor de 13 tools.

**Arreglo aplicado** (`065ed73`, pusheado a `origin/main`): se versionan los dos módulos **sin modificar su
contenido** (615 líneas nuevas). Verificación tras el fix, sobre un clon limpio nuevo:

| módulo | import | tools |
|---|---|---|
| `octave_mcp` | OK | 13 |
| `gazebo_core` / `gazebo_entities` / `gazebo_quantum` | OK | — |
| `bolsa_mcp` / `mcp_suelos` / `copasi_mcp` | OK | — |

> Nota: `gazebo_utils.run_gz` reintroduce la ruta parcial `"gz"` (el patrón `B607` que el WIP elimina en
> `hilbert_tool` y `mcp_suelos`). No es un bug hoy, pero es una inconsistencia con el resto del WIP.

---

## 3. Lo que el WIP hace bien (verificado, no inferido)

| cambio | verificación |
|---|---|
| `jobs.py`: `shell=True` → `shell=False` | correcto en seguridad (rompía el `&&`: C3-2, ya resuelto con la opción A) |
| `jobs.py`: `KeyError` explícito, `job.get("pid")`, type hints, `encoding="utf-8"` | `start_job("no_existe")` → `KeyError: "Trabajo 'no_existe' no encontrado"` |
| `jobs.py`: cierre del `log_file` en el camino de excepción | **sin fuga de fd**: 4 → 4 descriptores con 3 jobs |
| `mcp_suelos.py`: `timeout=15` en `requests` + `raise_for_status` + error por fuente | sin excepciones sin manejar |
| `mcp_suelos.py`: `tempfile.mkstemp` en vez de `/tmp/temp_script.m` + cleanup en `finally` | 0 temporales huérfanos |
| `mcp_suelos.py`: timeout de Octave | 3,0 s → `{"error": "Octave excedió el timeout de 3s", "returncode": null}` |
| `mcp_suelos.py`: `ensure_ascii=False` | correcto |
| `hilbert_tool.py`: `mkstemp` + `_octave_bin()` | transform `am_chirp` OK, 0 temporales huérfanos |
| `bolsa_mcp.py`: `fastmcp` → `mcp.server.fastmcp` | **MCP con dato real**: `AAPL` cierre 349,36 · volumen 16.723.148 · 2026-11-09 |
| `gazebo_*`: refactor a `gazebo_utils` | **MCP**: `gz_version` → `Gazebo Sim, version 8.15.0` |
| `copasi_mcp.py`: `mcp.run()` | equivalente (`stdio` es el default) |

---

## 4. C3-4 — COPASI: tools expuestas sin capacidad → operativo

**Diagnóstico original (parcialmente incorrecto).** La config desplegada pedía
`COPASI_BINDINGS_DIR=/home/astrid/COPASI/build/copasi/bindings/python`, que no existe, y `python3` del
sistema no tiene los bindings:

```
ModuleNotFoundError: No module named 'COPASI'
```

El server **arrancaba y degradaba bien** (`estado_copasi` → `"COPASI NO disponible"` + traceback), pero
las tools no podían simular nada. **Lo que faltaba era el entorno, no COPASI**: la instalación existe en
`/home/astrid/venv-copasi` (COPASI **4.48.311** + `mcp` + numpy/scipy/pandas/matplotlib/cffi).

**Config corregida** (desplegada y template):

```json
"copasi": {
  "command": "/home/astrid/venv-copasi/bin/python",
  "args": ["/home/astrid/Documents/OpenScience/projects/githu/mcp-ecosystem-chiloe/servidores-propios/copasi_mcp.py"],
  "env": { "COPASI_BINDINGS_DIR": "/home/astrid/venv-copasi/lib/python3.12/site-packages" }
}
```

**Bugs de API de COPASI 4.48 corregidos en `copasi_mcp.py`:**

| bug | síntoma | arreglo |
|---|---|---|
| `setChemEq()` no existe en 4.48 | error al armar el esquema | `setReactionScheme(esquema)` |
| `saveModel()` escribe **CopasiML**, no SBML | el modelo no se podía recargar | `_load_model()`: intenta `loadModel()` y, si no, `importSBML()` |
| al recargar se pierde el mapeo interno sustrato/producto → metabolito | simulación aborta con `"Invalid state"` y serie en NaN | `_reparar_mapeo_reacciones(model)`: re-aplica `setReactionScheme` + `setFunction` y restaura los valores finitos (los NaN se omiten) |
| `setInitialConcentration()` no persiste al guardar | concentraciones iniciales perdidas | `setInitialExpression(repr(valor))` **después** de `compileIfNecessary()` (los metabolitos tienen que existir antes) |
| la serie de salida viene en **nº de partículas** | el SSE comparaba partículas contra concentraciones | `_particulas_a_concentracion(valor, model)`: ÷ N_A (6.02214076e23) ÷ volumen del compartimento |

Orden correcto discovered por experimentación: **valor candidato → reparar modelo → compilar UNA vez →
`applyInitialValues()` → simular**. Aplicar la expresión inicial *después* de `setReactionScheme` y
recompilar deja el modelo en `NaN` ("Invalid state").

**Verificación (los 7 tools por MCP):** `estado_copasi`, `listar_modelos`, `construir_modelo_simple`,
`info_modelo`, `obtener_parametro`, `simular` y `ajustar_parametro_simple` responden OK.

**Verificación numérica** (no solo "no tira excepción"):

| prueba | resultado |
|---|---|
| cinética `A → B`, `k = 0.5`, `A0 = 10` | `A(10) = 0.0674` mol vs analítico `10·e^(-0.5·10) = 0.0674` → error relativo ~1e-5 |
| ajuste de `k1` sobre datos sintéticos | recupera **0.5** (SSE 5.6138e-07) y **0.25 exacto** (SSE 9.495e-12) |
| ajuste de `A0` | ≈ 9.4444 en grilla (valor real 10; la grilla es gruesa) |
| persistencia | `fijar_parametro(A=20)` → sobrevive al guardado/carga (`demo_ciclo6d`) |

---

## 5. Hallazgos menores

- **C3-5 colisión de `job_id`** — `create_job` usa `int(time.time())`; dos jobs `gromacs/mismo` en el mismo
  segundo dan el mismo id (`gromacs_mismo_1794247259`) y el segundo pisa al primero. Sugerencia: `uuid4().hex[:8]`.
- **C3-6 imports sin usar** — 6 F401 nuevos (`gazebo_entities`: `subprocess`, `shutil`; `gazebo_quantum`:
  `euler_to_quaternion`; `formulas_tool`: `sympy.Symbol`, `sympy.nsimplify`, `sympy.sympify`) sobre 5
  preexistentes (`gazebo_core`: `json`, `signal`; `gazebo_quantum`: `os`; `copasi`: `zip` sin `strict=`;
  `btfloat`: `random` no criptográfico). `ruff --fix` resuelve los 9 auto-corregibles.
- **C3-7 duplicación** — `_octave_bin()` quedó en `octave_runner.py`, `hilbert_tool.py` y `mcp_suelos.py`
  con el mismo cuerpo. Como `octave_runner.py` ya está versionado, los otros dos podrían importarlo.
- **C3-8 contrato** — `mcp_suelos.ejecutar_octave` ahora devuelve JSON (`stdout`/`stderr`/`returncode`) donde
  antes devolvía stdout crudo. Cualquier consumidor que espere texto plano tiene que actualizarse.
- **C3-9 clave TLS** — `mcp-ecosystem-chiloe.key` (clave privada de localhost, CN=localhost) estaba sin
  trackear **y sin regla de `.gitignore`**: un `git add -A` la publicaba. Ahora `*.key`, `*.crt`, `*.pem`,
  `*.p12` están ignorados (y se limpiaron dos líneas basura de un heredoc previo).

---

## 6. Cambios que aplicó el agente sobre el repo canónico

**`065ed73` → `origin/main`:**

1. `git add` de `servidores-propios/formulas_tool.py` y `servidores-propios/gazebo_utils.py` — **contenido sin
   modificar**; solo se registran en el control de versiones porque el repo no funcionaba sin ellos.
2. `.gitignore`: `*.key`, `*.crt`, `*.pem`, `*.p12` + limpieza de dos líneas basura.

**`8bfd6ba` → `origin/main` (fixes propios):**

3. `server.py`: `run_gromacs` en dos etapas (grompp síncrono con timeout de 300 s y log propio; mdrun como
   job en background con `argv` list y `shlex.quote` por token). `parse_results` pasa `job.get("cwd")`.
4. `parsers.py`: `GROMACS_LOG_NAMES`, `parse_gromacs_log(log_path, cwd=None)`, clave `log_file`, regex de
   error real y parseo del `Performance` de GROMACS 2018+.
5. `servidores-propios/copasi_mcp.py`: compatibilidad con COPASI 4.48.311 (tabla de §4), `_load_model`,
   `_reparar_mapeo_reacciones`, `_particulas_a_concentracion`, `estado_copasi` reporta versión + módulo +
   directorio de bindings; `COPASI_BINDINGS_DIR` sin fallback hardcodeado.
6. `claude_desktop_config.template.json`: entrada `copasi` con el venv + `COPASI_BINDINGS_DIR`.
   (Sobre este archivo venía tu WIP; se respetó salvo ese ajuste.)

**`3bf3141` → `origin/main` (tu WIP, byte a byte):** `jobs.py`, `verificar_mcp.py` y
`servidores-propios/{bolsa_mcp.py, btfloat/btfloat.py, gazebo_core.py, gazebo_entities.py,
gazebo_quantum.py, hilbert_tool.py, mcp_suelos.py}`. **Sin reescribir**: C3-5, C3-6 y C3-7 siguen
pendientes y así quedan documentados.

**Fuera de los commits** (untracked preexistentes, anteriores al ciclo de auditoría): `notes/`,
`knowledge/`, `innovative_test.py`, `test_alternatives.py`, `test_final.py`, `test_tool.py` y
`servidores-propios/{alternatives_mcp.py, auto_mcp_config.py, fix_motion.py, github_sync.py,
regenerate_config.py}`.

**Fuera del repo canónico pero tocados:** `/home/astrid/.config/Claude/claude_desktop_config.json`
(entrada `copasi` apuntando al venv) — la config desplegada está fuera de git por diseño
(`.gitignore`: `claude_desktop_config.json`).

---

## 7. Limitaciones

- `md-simulation` **no está en la config desplegada**: los fixes de GROMACS se verificaron por MCP
  directo (Inspector), no en producción. Si lo agregás a la config, probá `run_gromacs` una vez.
- El A/B de GROMACS usó un sistema mínimo de un átomo con `nsteps=0` para el pipeline de comandos, y un
  run de 200 pasos para la lectura del log: prueba el **pipeline y el parseo**, no la física de la
  simulación.
- `gdal`, `qgis` y `openbb-mcp` siguen sin poder verificarse offline (requieren `uvx` con red).
- COPASI, GROMACS y LAMMPS se verificaron con las versiones instaladas en esta máquina (COPASI 4.48.311,
  GROMACS 2023.3, Gazebo Sim 8.15.0); el comportamiento puede diferir en otros sistemas. Los fixes de
  COPASI son **específicos de 4.48** (`setReactionScheme`): en una versión anterior que sí tenga
  `setChemEq` habría que mantener las dos rutas.
- La serie de COPASI que consume el ajuste depende del volumen del compartimento del modelo: el helper
  `_particulas_a_concentracion` lee ese volumen del modelo cargado, así que un modelo con compartimentos
  múltiples o unidades distintas necesita revisión.
- La config desplegada tiene 22 servidores y hay **decisiones de ciclo 5 sin cerrar** (sustituir `octave`
  viejo por `octave-chiloe`, si entran `bench` / `mcp-blender-full` / `unrealMCP`) y hace falta
  **reiniciar Claude Desktop** para que los servidores nuevos/cambiados se carguen.
