# Análisis estático de `octave-mcp` vía VS Code MCP

**Repositorio analizado:** `https://github.com/Astrid3333/octave-mcp.git`
**Clon local:** `/home/astrid/octave-mcp` (rama `migracion-mcp-octave-real`, HEAD `5d84431`, 4 commits por delante de `origin/migracion-mcp-octave-real`)
**Fecha:** 2026-11-08
**Herramientas:** VS Code 1.145.2 + MCP Bridge 4.9.5 (`vscode_get_diagnostics`, `vscode_open_files`, `vscode_get_symbol_lsp_info`), con dos analizadores independientes dentro del mismo editor: **Pylance** (`ms-python.vscode-pylance` 2026.4.1) y **Ruff** (`charliermarsh.ruff` 2026.94.0). Contrastados con `ruff` 0.15.19 en CLI y con ejecución directa de los módulos.

---

## 1. Alcance y método

1. Se abrió el clon como carpeta en VS Code (el bridge registra un workspace por ventana) y se comprobó con `vscode_list_workspaces` que quedaron **2 workspaces activos**: `octave-mcp` y el workspace propio `githu`.
2. Control de validez: se creó un archivo temporal con dos errores deliberados (`"texto" + 1` y `os.no_existe`). Pylance vía MCP los reportó correctamente → el canal funciona. El archivo se eliminó después.
3. Limitación operativa descubierta: **`vscode_get_diagnostics` solo devuelve diagnósticos de archivos abiertos en el editor**. Para Python, un archivo no abierto devuelve "clean" aunque tenga errores. Por eso cada archivo se abre primero con `vscode_open_files`. Además, las consultas con más de ~4 archivos agostan el timeout de 30 s, así que se consultaron en lotes de 2–3.
4. **Convención de líneas (importante):** las posiciones que devuelve el MCP (`vscode_get_diagnostics`) siguen la API de diagnósticos de VS Code, que usa **líneas y columnas 0-based**. Para obtener la línea real del archivo hay que sumar 1. Todas las referencias de este informe están ya en **1-based** y se verificaron con `grep -n` y con `ruff` en CLI (1-based). Ejemplo: MCP reporta `plotting_tools.py` en `522:10`; la línea real es **523:11**.
5. Cobertura: los **19 archivos del manifiesto estricto del proyecto** (`ci/ruff_files.txt`) + `ci/smoke.py` + `tests/test_math_pipeline_builder.py` + una **muestra dirigida de 13 archivos legacy** seleccionados por `ruff` por contener los códigos más graves (`F821` nombre indefinido, `F811` redefinición).

## 2. Estructura del repositorio (verificada)

| Ruta | Contenido | ¿Lo carga `server.py`? |
|---|---|---|
| raíz | 472 `.py` (214 de ellos módulos `_tool` importados explícitamente) | **sí**, por `import <modulo>` explícito |
| `tools/` | 91 `.py` | **no** (0 referencias a `import tools` / `from tools`; sin descubrimiento dinámico: no hay `importlib`, `pkgutil`, `glob` ni `listdir` en `server.py`) |
| `submotors/` | 6 `.py` (1210 archivos en total, mayoría datos) | no |
| `engines/` | `rk4_verify.cpp`, `rk4_verify.rs` | no |
| `tests/` | `test_math_pipeline_builder.py`, `pytest.ini` (marcador `integration`) | n/a |

`server.py` es el bootstrap stdio del servidor MCP (1315 líneas). La rama `migracion-mcp-octave-real` sugiere una migración en curso de módulos a la raíz; `tools/` parece ser la copia **anterior**, pero no es un duplicado exacto: las versiones de raíz y de `tools/` difieren en 133–181 líneas por archivo.

**CI (`.github/workflows/ci.yml`):** dos jobs — `lint` (ruff 0.15.19 sobre solo el manifiesto `ci/ruff_files.txt`, 19 archivos) y `smoke` (`pip install -r requirements.txt` + `python ci/smoke.py`, que espera 372 tools y 2 validaciones puras).

## 3. Diagnósticos por archivo (Pylance + Ruff vía MCP)

### 3.1 Manifiesto estricto de CI (19 archivos, ruff-clean por diseño)

| Archivo | Total | ERROR | HINT/WARN | Naturaleza |
|---|---|---|---|---|
| `server.py` | 0 | 0 | 0 | limpio |
| `tool_registry.py` | 0 | 0 | 0 | limpio |
| `cas_router_math_tool.py` | 0 | 0 | 0 | limpio |
| `sympy_eval_tool.py` | 0 | 0 | 0 | limpio |
| `run_validations_guarded.py` | 0 | 0 | 0 | limpio |
| `pdelta_tool.py` | 0 | 0 | 0 | limpio |
| `timber_design_tool.py` | 0 | 0 | 0 | limpio |
| `workspace_tool.py` | 2 | 0 | 2 | `_validate` y `args` sin usar |
| `composite_homogenization.py` | 3 | 0 | 3 | `name`/`schema`/`handler` sin usar |
| `health_check_tool.py` | 3 | 2 | 1 | unión de tipos en el dispatcher |
| `math_pipeline_builder_tool.py` | 3 | 2 | 1 | `steps: list = None`; `**dict` sin tipar |
| `concrete_design_tool.py` | 6 | 6 | 0 | unión `str | float` en el dispatcher |
| `materials_db_tool.py` | 8 | 8 | 0 | `**dict` sin tipar; `.get` sobre unión |
| `fire_design_tool.py` | 11 | 10 | 1 | unión `str | float` en el dispatcher |
| `octave_infra_tool.py` | 12 | 12 | 0 | `str = None` por defecto; retorno `str | dict` |
| `eurocode_steel_tool.py` | 15 | 8 | 7 | unión `str | float`; constantes sin usar |
| `examples/multi_cas_crosscheck.py` | 18 | 18 | 0 | stubs de sympy; `proc.stdin` Optional |
| `statistics_extended_tool.py` | 24 | 20 | 4 | stubs de scipy (`chi2_contingency`) |
| `run_all_validations.py` | 4 | 2 | 2 | `dict` asignado a `Dict[str, str]` |
| **Total** | **109** | **88** | **21** | |

Ruff y Pylance coinciden en todos los archivos del manifiesto: **no hay un solo `F821`/`F811` en el conjunto que CI lintea estrictamente**. Los 88 ERROR de Pylance son, en su gran mayoría, de dos clases que ruff no cubre: contrato de tipos (§4.5, §4.6) y stubs de sympy/scipy (§4.7).

### 3.2 Muestra legacy (fuera del manifiesto)

| Archivo | Total | ERROR | Observación |
|---|---|---|---|
| `algae_chemostat_tool.py` | 17 | 3 | `json`/`compute_dispatcher` indefinidos + 10× E702 |
| `lichen_growth_tool.py` | 15 | 3 | mismo patrón `__main__` + `rate_initial` sin usar |
| `moss_lsystem_tool.py` | 14 | 3 | mismo patrón `__main__` + 8× E702 |
| `cryptogam_biomass_tool.py` | 11 | 3 | mismo patrón `__main__` + 5× E702 |
| `photosynthesis_lichen_tool.py` | 10 | 3 | mismo patrón `__main__` |
| `plotting_tools.py` | 19 | 4 | `plt` indefinido (§4.1) + E731 |
| `tools/ion_chemistry_tool.py` | 18 | 6 | `TOOL_SCHEMA` indefinido (§4.2) + 4× `reportOptionalOperand` |
| `tools/geospatial_risk_analysis_tool.py` | 21 | 15 | `validate` redefinido (§4.4) + `reportReturnType` |
| `tools/wildfire_intensity_model_tool.py` | 9 | 3 | `validate` redefinido (§4.4) |

**Deuda global (ruff CLI, todo el repo, `--select F,E9`): 299 hallazgos** — `F401` 129 (import sin usar), `F841` 99 (local sin usar), `F541` 36 (f-string sin placeholders), **`F821` 27 (nombre indefinido)**, `F811` 6 (redefinición).

## 4. Hallazgos clasificados

### 4.1 H1 — BUG REAL y alcanzable: `plotting_tools.py` usa `plt` sin importarlo

**Severidad: alta (rompe un modo de una tool registrada).**

`plotting_tools.py:520-542`, función `_render_png_base64_3d`:

```python
def _render_png_base64_3d(x, y, z, point, T, N, B, scale=None):
    from mpl_toolkits.mplot3d import Axes3D  # noqa: F401  (registra proyeccion 3d)
    fig = plt.figure(figsize=(6, 6))       # <-- plt NO definido
    ...
    fig.savefig(buf, format="png", dpi=110, bbox_inches="tight")
    plt.close(fig)                          # <-- plt NO definido
```

El módulo **no importa matplotlib a nivel de raíz** (líneas 33-40: solo `base64`, `io`, `numpy`, `sympy`, `scipy.integrate`, `sys`, `json`). La función姊妹 `_render_png_base64` (línea 111-115) sí hace el import local correcto:

```python
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
```

`_render_png_base64_3d` no lo replica → **`NameError: name 'plt' is not defined`** en cuanto se ejecuta. Es alcanzable: `plotting_tools.py:515` la invoca desde el flujo de render 3D de campo vectorial (`result["png_base64"] = _render_png_base64_3d(xs, ys, zs, point, T, N, B)`).

Confirmado por dos analizadores independientes vía MCP: Ruff `F821 Undefined name 'plt'` (líneas 523 y 540) y Pylance `reportUndefinedVariable` (523:11 y 540:5). El `# noqa: F401` del autor sobre el import de `Axes3D` muestra que el lint de esa zona se revisó a medias: se silenció el import sin usar, pero no se detectó el `plt` ausente.

**Corrección:** repetir el patrón de `_render_png_base64` (import local de `matplotlib` + `matplotlib.use("Agg")` + `import matplotlib.pyplot as plt`) dentro de `_render_png_base64_3d`.

### 4.2 H2 — BUG REAL (latente): `tools/ion_chemistry_tool.py` no se puede importar

**Severidad: media (archivo huérfano; no lo carga el servidor).**

`tools/ion_chemistry_tool.py:667`:

```python
try:
    from tool_registry import register_tool
    register_tool(TOOL_NAME, TOOL_SCHEMA, _dispatch)   # TOOL_SCHEMA no existe
except ImportError:
    pass
```

`TOOL_NAME` está definido (línea 640) pero **`TOOL_SCHEMA` no existe en el archivo**: la única aparición del símbolo es su uso en la línea 667. Un `NameError` **no** lo captura `except ImportError` → el import del módulo propaga la excepción.

Verificado por ejecución directa (no solo por análisis estático):

```
IMPORT FALLA: NameError name 'TOOL_SCHEMA' is not defined
```

La copia de la **raíz** (`ion_chemistry_tool.py`, la que sí importa `server.py:404`) sí define `TOOL_SCHEMA` (2 ocurrencias) y sí funciona. Consecuencia: `tools/ion_chemistry_tool.py` es una **copia divergente rota** (133 líneas de diferencia respecto de la raíz) que quedó sin migrar.

Confirmado por MCP: Ruff `F821` (667:30) y Pylance `reportUndefinedVariable` (667:30).

### 4.3 H3 — El subpaquete `tools/` es inejecutable por construcción

**Severidad: media-alta (116 archivos, 91 `.py`, no registrados en el servidor).**

Dos defectos independientes en `tools/__init__.py`:

**(a) `NameError` en el primer registro.** El archivo usa `tool_registry.register_tool(...)` (líneas 4, 13, 21, 30) sin haber importado `tool_registry` en su espacio de nombres. Verificado por ejecución:

```
$ python3 -c "import tools"      → IMPORT tools FALLA: NameError name 'tool_registry' is not defined
$ python3 -c "import tool_registry; import tools"
                                 → IMPORT tools FALLA: NameError name 'tool_registry' is not defined
```

Preinyectar `tool_registry` en `sys.modules` **no** lo arregla: el nombre debe existir como global del propio módulo `tools`. La entrada `except ImportError: pass` de cada tool tampoco ayuda.

**(b) Doble registro → `ValueError`.** `tools/__init__.py` importa y registra `ethnomath_comparative_tool` **dos veces** (bloques de líneas 12-19 y 20-27, con el mismo alias `COMP`). Y `tool_registry.register_tool` lanza excepción ante colisión de nombre:

```python
def register_tool(name, schema, handler):
    if name in REGISTRY:
        raise ValueError(f"Tool '{name}' ya esta registrada -- colision de nombre entre modulos. ...")
```

Es decir: aunque se arreglara (a), la segunda llamada al `register_tool` abortaría con `ValueError`. El defecto (b) está **latente** detrás del defecto (a).

Impacto real: ninguno hoy — `server.py` no importa el paquete `tools` (0 coincidencias de `import tools`/`from tools`, y no hay carga dinámica por `importlib`/`pkgutil`/`glob`). Pero son 91 módulos Python, con duplicados divergentes de la raíz, que **el servidor no registra** y que no se pueden importar para probarlos.

### 4.4 H4 — `validate` duplicado: 132 y 200 líneas de código muerto

**Severidad: media (mantenimiento).**

Dos archivos definen `validate` dos veces en el mismo módulo:

| Archivo | 1ª definición | 2ª definición | Registrado |
|---|---|---|---|
| `tools/geospatial_risk_analysis_tool.py` | línea 597 (132 líneas, "portado desde la version raiz") | línea 729 (200 líneas) | la 2ª |
| `tools/wildfire_intensity_model_tool.py` | línea 409 | línea 525 | la 2ª |

En Python la segunda definición **sustituye** a la primera: la 1ª queda muerta. En `geospatial_risk_analysis_tool.py` el `run()` sí despacha `mode == "validate"` → `n_passed = validate()` (línea 588), por lo que se ejecuta la **segunda** versión. Consecuencia práctica: cualquier corrección aplicada a la copia muerta no tiene efecto, y un test escrito contra la copia muerta pasa sin ejercitar nada.

Confirmado por MCP: Ruff `F811 Redefinition of unused 'validate' from line 597` y Pylance `reportRedeclaration` — *"Function declaration 'validate' is obscured by a declaration of the same name"* (597:5 y 409:5).

### 4.5 H5 — Contrato de tipos: parámetros `str`/`list` con valor por defecto `None`

**Severidad: media (riesgo real de `TypeError`, no bug actual).**

- `octave_infra_tool.py:48` `def octave_run(code: str = None, timeout: int = DEFAULT_TIMEOUT, mode: str = None)`
- `octave_infra_tool.py:69` `def octave_eval_expr(expression: str = None, ...)`
- `octave_infra_tool.py:87` `def octave_run_script(script_path: str = None, ...)`
- `octave_infra_tool.py:113` `def octave_version(mode: str = None)`
- `math_pipeline_builder_tool.py:171` `def run_math_pipeline(steps: list = None, mode: str = "validate") -> dict:`

El valor por defecto contradice la anotación y, si la función se invoca sin ese argumento, la primera operación que lo usa revienta: `octave_eval_expr` → `"disp(" + expression + ")"` (`TypeError: can only concatenate str (not "NoneType") to str`); `octave_run_script` → `Path(None)` (`TypeError`).

Pylance lo reporta como `reportArgumentType` (6 ocurrencias). Ruff no lo ve: no hay error de sintaxis ni de nombre.

**Corrección:** `code: str | None = None` con guarda explícita, o parámetro obligatorio.

### 4.6 H6 — Ruido dominante: el tipo de retorno de los dispatchers no está anotado

**Severidad: baja (calidad de tipos), pero explica ~60 de los 88 ERROR del manifiesto.**

Los dispatchers de las tools de diseño (`fire_design_tool.py:303`, `concrete_design_tool.py:353`, `eurocode_steel_tool.py:451`, `materials_db_tool.py:448`) y varios helpers de `health_check_tool.py` **no declaran tipo de retorno**. Pylance infiere la unión de todos los caminos de retorno y marca cada uso posterior como error:

```python
def _handler(args):        # sin anotación → Pylance infiere dict[str, str] | dict[str, Unknown]
    ...
r = _handler({"mode": "section_factor", "params": {...}})
ok = abs(r["k_AV_m1"] - k_av_exp) < 1e-12   # ERROR: Operator "-" not supported for "Unknown | str" and "float"
```

Evidencia directa con el LSP (hover sobre `_run_suite` en `health_check_tool.py`):

```
(function) def _run_suite(tools_filter: Unknown | None = None) -> (dict[str, str] | dict[str, Unknown])
```

Como el valor de la unión incluye `str` y `list`, cada `result["x"]["y"]` o `r["k"] - float` se vuelve "imposible" para el verificador, aunque en runtime el valor siempre sea el esperado. De ahí la cadena de errores en `health_check_tool.py:373-374` (`__getitem__` con clave `str` sobre una `list`).

**Corrección:** anotar `-> dict` (o `-> dict | str` si hay rutas de error) y estrechar con `isinstance` donde haga falta; alternativamente, que el dispatcher devuelva siempre un dict (`{"error": ...}`) en vez de mezclar `str` y `dict`.

Casos equivalentes: `materials_db_tool.py:287` y `:386` (`**r["properties"]` → *"Expected mapping for dictionary unpack operator"*), `run_all_validations.py:200,254` (`dict` asignado a `Dict[str, str]`), `tools/geospatial_risk_analysis_tool.py:112` (`reportReturnType`: declara `Dict[str, float]` pero devuelve `dict[str, float | str | dict[str, float]]`).

### 4.7 H7 — Falsos positivos por stubs de sympy/scipy (no tocar)

**Severidad: ninguna; documentar para no "arreglar" código correcto.**

- `statistics_extended_tool.py` (20 ERROR): `from scipy import stats as _sp` → `_sp.chi2_contingency(obs)` está tipado por scipy como tupla genérica `_T_co@tuple` → `expected.tolist()` falla el análisis (`Cannot access attribute "tolist" for class "object*"`), `float(dof)` falla, etc. En runtime `scipy.stats.chi2_contingency` devuelve un `Chi2ContingencyResult` con `.expected` como `ndarray`: **el código es correcto**.
- `examples/multi_cas_crosscheck.py` (18 ERROR): `type[UndefinedFunction]`, `ArrayDerivative | Basic | Derivative`, `.rhs` sobre uniones → limitation de los stubs de sympy, no del código.
- `plotting_tools.py:465-470`: `ArrayDerivative | Basic | Derivative` como argumento de `float()` → mismo origen (sympy).

**Acción:** no "corregir" esto; si se quiere silenciar, usar `# type: ignore` puntual o `typing.cast`, nunca cambiar la lógica.

### 4.8 H8 — `__main__` roto en 5 tools legacy

**Severidad: baja-media (rompe la ejecución standalone, no el servidor MCP).**

Cinco módulos raíz terminan con un bloque `if __name__ == "__main__":` que usa `json` y `compute_dispatcher`, **ninguno de los dos definido/importado** en el módulo:

| Archivo | Líneas | Nombres indefinidos |
|---|---|---|
| `algae_chemostat_tool.py` | 505-507 | `json`, `compute_dispatcher`, `json` |
| `cryptogam_biomass_tool.py` | 335-337 | ídem |
| `lichen_growth_tool.py` | 478-480 | ídem |
| `moss_lsystem_tool.py` | 565-567 | ídem |
| `photosynthesis_lichen_tool.py` | 311-313 | ídem |

`python algae_chemostat_tool.py` → `NameError: name 'json' is not defined`. El servidor MCP no ejecuta esos bloques (importa los módulos), por eso CI no lo detecta: **el gate de CI no cubre la ejecución standalone de las tools**.

**Corrección:** `import json` en el bloque y usar el nombre real del dispatcher de cada módulo (o borrar el bloque `__main__` si no es un entry point soportado).

### 4.9 H9 — Código muerto y ruido de lint (129 `F401` + 99 `F841`)

Ejemplos representativos detectados vía MCP (Pylance `HINT ... is not accessed` / Ruff `F841`):

- `eurocode_steel_tool.py`: `_GM2` (124), `_PSI1_B` (128) — constantes definidas y nunca usadas; parámetros `c` (132, 170) y `lead_a` (337, 358, 368) ignorados.
- `fire_design_tool.py:52`: `_K_CONST` sin usar.
- `workspace_tool.py:385`: `_validate` sin usar; `:611`: `args` sin usar.
- `composite_homogenization.py:171`: `name`, `schema`, `handler` sin usar.
- `health_check_tool.py:350`: `params` **no se usa** en `compute_health_check(mode="summary", params=None, subset=None)` → la tool acepta `params` y lo descarta (contrato engañoso para el cliente MCP).
- `run_all_validations.py:306` (`c_elapsed`), `:453` (`all_checks`); `statistics_extended_tool.py:270,445`.

`health_check_tool.py:349` merece atención: `params` ignorado significa que el cliente puede mandar cualquier cosa sin efecto y sin aviso.

### 4.10 H10 — El smoke test no es reproducible en este entorno (gap de entorno, no de código)

`python3 ci/smoke.py` → `SMOKE FAIL: initialize sin respuesta; stderr=`

Causa verificada: `import server` falla en 1,5 s con `ModuleNotFoundError: No module named 'ocas'`. De las 15 dependencias de `requirements.txt`, **7 no están instaladas** en este intérprete:

| Disponible | Ausente |
|---|---|
| numpy, scipy, sympy, scikit-learn, matplotlib, PyWavelets, pillow, requests | **ocas, opencv-python-headless, scikit-image, okada_wrapper, cypari, python-flint, symengine** |

El CI sí instala todo (`pip install -r requirements.txt` antes de `ci/smoke.py`), así que el fallo local es un **gap de entorno**, no un defecto del repositorio. Consecuencia para este análisis: **el estado funcional del servidor (372 tools, 2 validate) no pudo verificarse localmente**; todo lo anterior es análisis estático más verificación por ejecución de fragmentos aislados.

## 5. Qué NO puede concluirse

- El servidor MCP **no arrancó** en este entorno (§4.10): no se pudo comprobar el registro real de tools, el conteo de 372, ni ejecutar `health_check_tool` contra el servidor.
- La cobertura de Pylance es **por archivo abierto**, no sobre el repo completo: 472 archivos `.py` en la raíz, 91 en `tools/`. La muestra legacy (13 archivos) se eligió por gravedad de lint, no al azar; el resto de los archivos legacy **no** fue examinado con Pylance.
- Las Eclipse/scipy/sympy unions son ruido conocido de stubs; sin un `pyrightconfig.json` en el repo no hay configuración de Strict que exaggerar ni tapar el ruido.
- No se modificó **ningún** archivo de `octave-mcp`: este informe es de solo lectura.

## 6. Recomendaciones priorizadas

| # | Acción | Fichero | Esfuerzo |
|---|---|---|---|
| 1 | Importar `matplotlib.pyplot` localmente en `_render_png_base64_3d` (§4.1) | `plotting_tools.py` | 3 líneas — **arregla un bug alcanzable** |
| 2 | Definir `TOOL_SCHEMA` o borrar el bloque de registro roto (§4.2) | `tools/ion_chemistry_tool.py` | 1 línea o borrar archivo |
| 3 | Decidir el destino de `tools/`: migrar a raíz, arreglar el paquete (`import tool_registry` + eliminar el registro duplicado), o borrarlo (§4.3) | `tools/` | decisión de arquitectura |
| 4 | Eliminar la definición muerta de `validate` y quedarse con una (§4.4) | `tools/geospatial_risk_analysis_tool.py`, `tools/wildfire_intensity_model_tool.py` | borrar 132 + 116 líneas |
| 5 | Anotar los retornos de los dispatchers (`-> dict`) y estrechar con `isinstance` (§4.6) | 8–10 tools de diseño | ~1 h — reduce ~60 ERROR a 0 |
| 6 | Corregir los `= None` en parámetros `str`/`list` (§4.5) | `octave_infra_tool.py`, `math_pipeline_builder_tool.py` | ~10 min |
| 7 | Arreglar o eliminar los bloques `__main__` de las 5 tools legacy (§4.8) | 5 archivos | ~15 min |
| 8 | `import tool_registry` en `tools/__init__.py` + eliminar el bloque duplicado (§4.3b) | `tools/__init__.py` | 5 min |
| 9 | Añadir al CI un gate que ejecute cada tool en modo `validate` (no solo lint) — habría detectado H1 y H8 | `.github/workflows/ci.yml` | ~20 min |
| 10 | Instalar las 7 dependencias ausentes para poder reproducir el smoke localmente (§4.10) | entorno | `pip install -r requirements.txt` |
| 11 | Limpiar el código muerto del legacy (129 `F401` + 99 `F841`) de forma incremental, empezando por el manifiesto estricto | 472 archivos raíz | gradual |

## 7. Reproducibilidad

```bash
# 1. Workspace con el env correcto para el bridge (el path del socket Unix
#    supera 107 chars si XDG_DATA_HOME es el del workbench)
XDG_DATA_HOME=/home/astrid/.local/share code --new-window \
  --disable-gpu \
  --user-data-dir=/home/astrid/.local/share/com.ai4s.workbench/runtime/xdg-config/Code \
  /home/astrid/octave-mcp

# 2. Por cada archivo a analizar: abrirlo y consultarlo (los no abiertos
#    devuelven "clean" falseamente)
#    MCP: vscode_open_files(files=[...]) → vscode_get_diagnostics(filePaths=[...])
#    Lotes de 2-3 archivos: con más, agota el timeout de 30 s

# 3. Contraste en CLI
cd /home/astrid/octave-mcp
ruff check --config .ruff.toml --output-format concise --select F,E9 .
python3 -c "import tools"                       # -> NameError: tool_registry
python3 -c "import server"                      # -> ModuleNotFoundError: ocas (entorno)
```

**Nota sobre `--disable-gpu`:** sin él, la ventana de VS Code no abría renderer en esta máquina y el extension host no arrancaba (tardaba >3 min); con `--disable-gpu` el extension host levantó en ~2 min y el socket del bridge se creó correctamente.
