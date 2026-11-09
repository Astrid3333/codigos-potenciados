# Auditoría del WIP del usuario — `mcp-ecosystem-chiloe`

**Fecha:** 2026-11-09 · **Ciclo:** 6 (continuación del ciclo 5) · **Base auditada:** `982ab0f`
**Alcance:** los 11 archivos modificados sin commitear + los 12 untracked del repo canónico
`/home/astrid/Documents/OpenScience/projects/githu/mcp-ecosystem-chiloe`
**Método:** diff vs HEAD · ruff con atribución preexistente/introducido · grafo de imports vs `git ls-files` ·
clon limpio de `origin/main` · MCP Inspector 2.13.0 (`tools/list` + `tools/call`, secuencial, cwd `/tmp`) ·
A/B del pipeline GROMACS por MCP contra HEAD y contra el WIP.

Evidencia cruda: `wip_audit_evidence.json`.

---

## 0. Veredicto

El WIP **mejora** de forma real la robustez del código (timeouts, temporales seguros, fin de `shell=True`,
type hints, errores explícitos), pero trae **dos regresiones y una dependencia rota**:

| # | Severidad | Hallazgo | Estado |
|---|-----------|----------|--------|
| C3-1 | **alta** | `formulas_tool.py` sin versionar → el repo publicado no arranca en un clon limpio | **Corregido** (`065ed73`) |
| C3-2 | **alta** | `jobs.py` con `shell=False` + `shlex.split` **rompe `run_gromacs`** (el cmd lleva `&&`) | **Abierto — decisión tuya** |
| C3-3 | media | `gazebo_utils.py` sin versionar → el WIP de los 3 servidores gazebo no es reproducible | **Corregido** (`065ed73`) |
| C3-4 | media | COPASI: 9 tools expuestas pero el módulo no está instalado | **Abierto — decisión tuya** |
| C3-5 | baja | Colisión de `job_id` (mismo motor+nombre en el mismo segundo) | Abierto |
| C3-6 | baja | 6 imports sin usar nuevos (3 en modificados, 3 en el archivo nuevo) | Abierto (`ruff --fix`) |
| C3-7 | baja | `_octave_bin()` duplicado en 3 archivos | Abierto |
| C3-8 | info | `mcp_suelos.ejecutar_octave` cambia su contrato (JSON en vez de stdout crudo) | A documentar |
| C3-9 | info | Clave TLS privada sin trackear **y sin regla de ignore** | **Corregido** (`065ed73`) |

Además, el WIP es **mejor que HEAD en 10 puntos verificados** (§3), incluido el fix real de
inyección de shell en `jobs.py` — que es justo lo que causa C3-2.

---

## 1. C3-2 en detalle — por qué `shell=False` rompe GROMACS

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
| `jobs.py`: `shell=True` → `shell=False` | correcto en seguridad (rompe el `&&`: C3-2) |
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

## 4. C3-4 — COPASI: tools expuestas sin capacidad

La config desplegada apunta a `COPASI_BINDINGS_DIR=/home/astrid/COPASI/build/copasi/bindings/python`,
**ese directorio no existe** y no hay instalación de COPASI en el sistema:

```
ModuleNotFoundError: No module named 'COPASI'
```

El server **arranca y degrada bien** (`estado_copasi` devuelve `"COPASI NO disponible"` + traceback), pero
las 9 tools de `copasi` no pueden simular nada. Opciones: instalar COPASI con sus bindings Python, o sacar
`copasi` de la config desplegada y del template.

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

## 6. Cambios que aplicó el agente (únicos sobre el repo canónico)

Commit `065ed73` → `origin/main`:

1. `git add` de `servidores-propios/formulas_tool.py` y `servidores-propios/gazebo_utils.py` — **contenido sin
   modificar**; solo se registran en el control de versiones porque el repo no funcionaba sin ellos.
2. `.gitignore`: `*.key`, `*.crt`, `*.pem`, `*.p12` + limpieza de dos líneas basura.

**No se tocó tu WIP de código** (`jobs.py`, `server.py`, `mcp_suelos.py`, `hilbert_tool.py`, `gazebo_*`,
`copasi_mcp.py`, `bolsa_mcp.py`, `btfloat.py`, `verificar_mcp.py`, `claude_desktop_config.template.json`):
sigue exactamente como lo dejaste, sin commitear.

---

## 7. Limitaciones

- `md-simulation` **no está en la config desplegada**: la regresión C3-2 no afecta a nada en producción hoy;
  te va a morder cuando commitees el WIP o lo agregues a la config.
- El A/B de GROMACS usó un sistema mínimo de un átomo con `nsteps=0`: prueba el **pipeline de comandos**, no
  la física.
- `gdal`, `qgis` y `openbb-mcp` siguen sin poder verificarse offline (requieren `uvx` con red).
- COPASI, GROMACS y LAMMPS se verificaron con las versiones instaladas en esta máquina (GROMACS 2023.3,
  Gazebo Sim 8.15.0); el comportamiento puede diferir en otros sistemas.
