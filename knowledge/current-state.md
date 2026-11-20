# Current State

- Goal: **ciclo 19** — medir con evidencia los generadores de 64 bits del repo
  (`msws64`, `hybrid_prng`, híbrido LCG+msws64) con la misma batería que
  `randu_plus_validator.py`, y con controles limpio (`/dev/urandom`) y
  defectuoso (estado crudo de `randu_plus`).
- Stage: **ciclo 19 cerrado y verificado** (nota completa en `notes/2026-11-20.md`).
  - **Nuevo**: `prng_modules_validator.py` (compila drivers C para los tres
    módulos, 8 semillas × 7 tests = 56 p-values por generador, 1M uint32 por
    semilla, descomposición natural little-endian) → `VALIDATION_REPORT_PRNG_MODULES.md`.
  - **Resultados**: `msws64` 0/56 p<0.01 y 0/56 p<0.001; `hybrid_prng` 1/56
    (NIST_BlockFreq, semilla 65537, p=0.00757203, **reproducible**); LCG+msws64
    0/56; control limpio `/dev/urandom` 1/56; control negativo 15 planos,
    crudo p=0.0, mezclado p=0.6935, unmix32 OK.
  - **Calibraciones que hicieron falta**: p-value exacto (`chi2_sf_exact`) en
    permutation/block-freq (Wilson-Hilferty daba falsos FAIL con df≈320);
    descomposición natural (hi/lo intercalado daba falsos FAIL); test espectral
    de Marsaglia con **200 000 tuplas** (con 50 000 el p-value fluctúa: mismo
    flujo → 0.000432 vs 0.0751181; el FAIL era ruido del test); criterio por
    **conteo de falsos positivos** (H0: 0.56 de 56; límite 99,9% = 4) en vez de
    "los 56 deben pasar".
  - Cambios: `randu_plus_validator.py` (Marsaglia a 200000 tuplas, control
    negativo a 600000 valores), `README.md` (tabla de la medición + salvedades
    honestas), `VALIDATION_REPORT.md` regenerado (**10/10**),
    `VALIDATION_REPORT_PRNG_MODULES.md` nuevo. `cc -std=c99 -Wall -Wextra
    -pedantic -Werror randu_plus.c` → 8/8; `ruff` limpio en el script nuevo.
  - **Pendiente**: push (nunca solicitado explícitamente) y refreshed de las
    copias `pasted*.txt` para el revisor (no puede abrir URLs de GitHub).
- Goal anterior: **ciclo 17** — prueba directa del retículo de RANDU pedida por el
  revisor: pegar `randu_plus.h` y `randu_plus_validator.py` (el usuario no
  puede abrirlos en GitHub) y medir, no suponer, la estructura de planos y el
  test espectral 3D sobre la salida **cruda** vs. la **mezclada**.
- Stage: **ciclo 17 cerrado y verificado** (nota completa en `notes/2026-11-20.md`).
  - **Hechos medidos**: la relación `x[n+2] = 6·x[n+1] - 9·x[n] (mod 2^32)` se
    cumple **exactamente** en todas las ternas del estado crudo y el retículo
    ocupa **15 planos** (k = -9..5), los mismos 15 del RANDU clásico de Knuth
    mod 2^31 (medido también: 15). `unmix32(salida)` devuelve el estado crudo
    terna a terna → el retículo es recuperable desde la salida.
  - **Test espectral real de Marsaglia** (rejilla 16³, chi-cuadrado de
    ocupación sobre celdas completas, p-value exacto con gamma incompleta):
    estado crudo → chi2=22672, df=4095, **p=0**; salida mezclada → chi2=3969,
    df=4095, **p=0.919**.
  - **Punto ciego encontrado**: el `Spectral3D` que ya tenía el validadro NO es
    el test de Marsaglia (es el determinante de covarianza, sin p-value) y
    sobre el estado crudo de **esta** variante mod 2^32 da ratio≈0.99
    (**aprueba**), aunque sí rechaza el RANDU clásico mod 2^31 (ratio 0.0155):
    los planos de la variante 2^32 están más separados en el cubo unitario.
  - Cambios: `randu_plus_validator.py` (`chi2_sf_exact`+`gammaincc`,
    `marsaglia_spectral_test`, `lattice_plane_count`, `fmix32_unmix32`,
    `lattice_negative_control`, test `MarsagliaSpectral3D` en el barrido,
    sección de control negativo en el informe, caveat corregido); `randu_plus.h`
    (las 2 copias, byte a byte) documenta los 15 planos medidos; `README.md`
    describe el control negativo; `VALIDATION_REPORT.md` regenerado.
    Suite **10/10** (peor caso Marsaglia 0.106094, semilla 1); `ruff` con los
    mismos 10 hallazgos preexistentes (0 nuevos).
- Goal anterior: **ciclo 16** — cerrar la revisión externa del repo con 4 puntos:
  (1) batería estadística **multi-semilla** con criterio de peor caso,
  (2) integrar la evidencia de **TestU01 SmallCrush**, (3) corregir el dato de
  consumo de muestra del informe, (4) docs (`README` + descripción de GitHub).
- Stage: **ciclo 16 cerrado y verificado** (nota completa en `notes/2026-11-20.md`).
  - `randu_plus_validator.py`: barrido de **8 semillas impares** (12345, 1, 3, 7,
    4242, 65537, 99991, 2147483647) × 6 tests con 1M uint32 (4 MB) por semilla;
    el criterio de paso es el **mínimo entre semillas** (peor caso), no el de una
    ejecución afortunada. Cada test reporta su consumo real (`n_values`,
    `n_bits`). Peor caso: Spectral3D 0.982369, BirthdaySpacings 0.149875,
    Permutation 0.024850, NIST_Frequency 0.034827, NIST_BlockFreq 0.145194,
    NIST_Runs 0.255144. Suite **8/8** (incluye `HeaderCrossCheck` y `HeaderSync`).
  - **TestU01 SmallCrush** integrado vía el harness del usuario
    (`codigos-potenciados/testu01/run_randu_plus`, TestU01 1.2.3 enlazado
    estáticamente; **fuentes versionadas**, binarios ignorados): modo `fmix`
    pasa **15/15** en las 8 semillas; modo `state` (estado crudo del LCG)
    **falla 14/15** con p ~0.
    `fmix32` borra la estructura observable pero no la elimina (biyectivo) y el
    período sigue 2³⁰. Si el harness no está, el validador omite la sección.
  - **Poder discriminante corregido**: `BirthdaySpacings` usaba 5000 muestras en
    2²⁴ (0.745 colisiones esperadas → p ≈ 1.0000, test inútil); con 20000
    muestras (11.92 esperadas) el p varía 0.15–0.95. El 0.0398 que señalaba la
    revisión era un artefacto de esa configuración.
  - `VALIDATION_REPORT.md` regenerado: peor caso por test, matriz valor×semilla,
    consumo real de muestra por test, sección TestU01 y caveat reescrito
    (multi-semilla, consumo real vs. los 100 MB antes anunciados, qué detecta
    TestU01, harness versionado pero sin binario, reproducibilidad).
  - `README.md`: sección `randu_plus` (API + cómo reproducir) y typo
    `números_use` corregido. Descripción del repo en GitHub corregida (ya no
    dice "corregidos matemáticamente").
  - Push **hecho** por pedido explícito del usuario; `codigos-potenciados/testu01/`
    **versionado** por decisión suya (fuentes sí, binarios no).
- Goal anterior: **ciclo 10** — aplicar la revisión externa (`pasted.txt`) a 4 puntos del repo:
  (1) `sine_fixed.c` roto, (2) `randu_plus` no arregla RANDU, (3) `revivir.c` documentado
  con `frexp/ldexp` y atribución de Aryabhata, (4) el doc de Goertzel con umbrales medidos.
- Stage: **ciclo 10 cerrado y verificado** (nota completa en `notes/2026-11-19.md`).
  - `sine_fixed.c` (raíz) reescrito: fase entera en 1/100 grado, tabla de 3600 puntos con
    4 bits decimales extra, interpolación redondeada una vez, salida `int32_t` Q16.16
    completa. max_err **8.086e-06** (0.53 LSB). Test con exit code + **3 mutaciones
    detectadas**.
  - `codigos-potenciados/sine_fixed.c`: bug real = índice por coma flotante
    (`(int)(th/(2π/36000))` redondeaba a `i-1` en 2799/36001 puntos) → índice entero
    `(i + offset) mod 36001`. Implementado el **Q32.32** que el header prometía.
    Errores: Q16.16 **7.629e-06**, Q32.32 **1.164e-10**.
  - `randu_plus.{h,c}`: fmix32 **es** biyectivo → el retículo de RANDU se recupera desde
    la salida; período real **2³⁰** (verificado por exponenciación binaria). Header honesto
    (ejemplo histórico; para generar, `msws64/`). Test con **8 comprobaciones** y exit code.
    El inverso de `x ^= x>>13` es `x ^= x>>13; x ^= x>>26`.
  - `revivir.c`: solo docs (el `INV_M12_M3 = 28900` del pegado ya estaba bien desde
    `4e371a0`). Atribución de Aryabhata suavizada. Sin cambio de comportamiento.
  - Goertzel: **híbrida de 3 zonas** (α / K / β con cortes en π/4 y 3π/4) **+ plegado por
    aliasing** (`w' = 2π-w`, conjugando) en C, Python y Octave. Peor error en [0,2π):
    **2.215e-09 → 1.424e-10** (×15.6); error medio 1.312e-11 → **4.557e-12** (×2.9).
    El peor caso sin plegado está en `w → 2π` (no en todo `w > π`): allí `cos(w) → 1`
    y **K y β degeneran a la vez** (K→2, β→4); α no (`α=4sin²(w/2)→0`). El plegado lleva
    `w→2π` a `w'→0`, donde la híbrida elige α. C ↔ Python ↔ Octave **bit a bit**
    (dif máx 0.000e+00). Suite **29/29** (antes 24/24). Archivos nuevos
    `goertzel_umbrales.c` y `goertzel_umbrales_hibrida.c` (este último registra el
    `argmax` y el error medio).
  - **Bug real encontrado en `goertzel_test.m`**: en Octave `for v = x` itera por
    **columnas**, no por elementos → la recurrencia se aplicaba a un vector. Corregido a
    `for idx = 1:numel(x)`. El `.m` nunca se había ejecutado.
  - Docs: `lista_codigos_obsoletos.md` → `resultados_goertzel.md`; README raíz, README del
    módulo Goertzel y `msws64/README.md` actualizados.
- Next action: (a) commit local del cierre de `VALIDATION_REPORT.md` y la nota del ciclo
  (el resto del ciclo 10 ya esta en `1a0a2dd`); (b) proximo ciclo: hacer que
  `randu_plus_validator.py` compile un `.c` que incluya `randu_plus.h` en vez de escribir
  su propia copia embebida del generador (hoy divergen desde la salida 0 con semillas
  pares); (c) revisar `codigos-potenciados/goertzel/src/goertzel_arduino.h` (variante
  Arduino, su híbrido sigue con corte en π/2); (d) en `octave-mcp`: decidir si
  `ancestral_octave_tool.py` vuelve a `source()` de los `.m` o se regenera la copia
  embebida, y restaurar `mode=validate`.

- Goal: cerrar los **4 huecos** entre el benchmark (`lcl-bench`) y el repo de C
  (`githu/codigos-potenciados/`): (1) π por polígonos, (2) ábaco/suanpan + multiplicación védica,
  (3) middle-square de 4 dígitos, (4) entradas de registry para `CORDIC` / `Hastings` /
  `LCG-Lehmer` / `matematica_ancestral` + versión C de `goertzel_vs_fft`.
- Stage: **los 4 huecos cerrados y verificados**. El 4a (los 4 registries nuevos en `lcl-bench`)
  quedó commiteado localmente como `d92fcb4`: `cordic`, `hastings`, `lcg_lehmer` y
  `matematica_ancestral`, con evaluadores Octave (`octave/registry_eval_*.m`) y Python
  (`python/registry_run.py`), dispatch por `family` y los 4 archivos en
  `mcp/bench_server.py::_load_entries()`. **Octave 32/32 PASS y Python 32/32 PASS** con valores
  idénticos entre lenguajes; `hub_compare.py` exit 0 (las entradas nuevas salen
  `MISSING in fortran` porque el runner Fortran solo carga 3 registries).
  El 4b (`goertzel_vs_fft` en C, con FFT radix-2 real) también está implementado y documentado.
  Los módulos nuevos compilan con **0 warnings** bajo
  `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual
  -Wstrict-prototypes -Wmissing-prototypes -Werror` y pasan sus tests.
- Current task: ciclo 9 cerrado — los 4 registries nuevos de `lcl-bench`
  (`cordic`, `hastings`, `lcg_lehmer`, `matematica_ancestral`) tienen **tres**
  implementaciones: Octave, Python y **Fortran** (módulo nuevo
  `fortran/fmath.f90` + 4 evaluadores en `fortran/frunner.f90`, commit
  `4d9a4db`, pusheado). Fortran 28/28 entries (ASAN+UBSan limpio), Octave
  32/32, Python 32/32, hub con las 4 entradas en **ALL_PASS [PPP]** y
  coincidencia bit a bit entre lenguajes (1 ULP como máximo en
  `egyptian_sum_err`). Se corrigió además un dato de referencia del registry:
  `q15_err_max` decía "Medido: 4.509e-05" y los tres lenguajes miden
  **2.256e-05**. Ciclo 8 (fix de `hastings`, signo de sin y Remez) verificado
  con 33/33 y 0 warnings, commit `faf8f37`. Todos los commits de esta sesión
  están **pusheados** (`githu` y `lcl-bench` sincronizados con `origin`).
- Next action: (a) opcional: evaluadores Fortran para `goertzel_vs_fft` (hoy es
  un programa aparte, `fortran/registry_eval_dsp.f90`, no integrado al `f_runner`),
  `archimedes_pi`, `middle_square_weyl` y `hybrid_prng`; (b) con el usuario:
  reiniciar Claude Desktop para que `analisis-suelos` tome el `mcp_suelos.py`
  nuevo; (c) pendiente known: los 3 servidores `uvx` que requieren red
  (`gdal`, `openbb-mcp`, `qgis`).
- Ciclo 6 (cerrado antes): las 12 correcciones de Ternario están aplicadas y commiteadas en
  `Ternario-ancestral-sensores` (`master` @ `b361241`, fix de código en `544f633`, pusheado).
  Verificado: gcc `-m32 -Wall -Wextra` sobre las 45 TUs del kernel con 0 errores y con las
  clases de aviso que eran bug real en 0 (`-Wtype-limits` 9→0, `-Wimplicit-function-declaration`
  5→0, `-Wuninitialized`/`-Wmaybe-uninitialized` 2→0, `-Wpointer-to-int-cast` 2→0; total
  89→68, el resto `unused-*`/`sign-compare`); `make all` produce ISO + disk.img y el kernel
  bootea en QEMU; el userspace compila; los 2 experimentos Octave corren; `py_compile` OK y
  ruff sin hallazgos nuevos. El informe se actualizó con la sección "0. Estado tras la corrección".

## Facts_changed

- `hastings` (fix del 2026-11-14): `hastings_sin` reducía a [0, π/2] con `x = π - x` **y**
  `sign = -sign`, pero `sin(π - x) = sin(x)` conserva el signo → devolvía −0.866 para
  `sin(2π/3)` y +1 para `sin(3π/2)` (error máx 2.0 en [0, 2π]). Reducción correcta:
  `x > π → x = 2π - x` (signo invertido), `x > π/2 → x = π - x` (signo conservado). Error máx
  ahora **5.626e-08** en [0, 2π] y [−2π, 0]. El Remez seleccionaba los extremos alternantes
  **sin ordenar** (extremos del intervalo al final) y buscaba máximos con signo en vez de
  `|error|` → no convergiu a minimax; ahora ordena candidatos y alterna en orden creciente,
  exige ≥3 rondas y reporta el error real medido. Test del módulo: **33/33**.
- **Grado de los polinomios de Hastings**: `sin` y `atan` usan `x·P₅(x²)` = **grado 11 en x**;
  `exp` es grado 6 (`1 + x·P₅(x)`) y `log1p` grado 10 (`x·P₉(x)`). Un Remez de grado 5 no
  puede mejorarlos (minimax de sin en [0, π/2] con grado 5 = 7.71e-06 según `mpmath.chebyfit`,
  contra 5.63e-08 de Hastings). Con el grado equivalente el Remez da: sin **3.99e-14**,
  atan 1.71e-09, log1p 7.09e-10, exp 4.03e-08 (vs 3.89e-08 de Hastings: el Vandermonde en
  base monomial pierde precisión).
- Módulos nuevos en `codigos-potenciados/` (verificados el 2026-11-14, 0 warnings con flags
  estrictos y tests en verde): `pi_ancestral/` (**30/30**), `abacus_vedic/` (**29/29**),
  `middle_square/` (**17/17**) y `goertzel/` + `goertzel_vs_fft_test.c` (**24/24**).
- `pi_ancestral`: la recurrencia correcta de Arquímedes es `c' = 2·i·c/(i+c)` (media armónica) y
  `i' = i·sqrt(2·c/(i+c))` con **los valores viejos** (módulo: `i(10) = 3.141592516692157`,
  `c(10) = 3.141592927385096`). El **orden importa**: `c'` primero y después `i' = sqrt(c'·i)` es
  algebraicamente equivalente (converge a π, err 1.3e-15 a 30 iteraciones — es lo que hace la
  herramienta `archimedes_pi` de `mcp-ecosystem-chiloe`), pero si el inscrito se actualiza antes
  que el circunscrito la iteración colapsa a **3.300413165133028** en lugar de a π.
- `middle_square` (von Neumann, 4 dígitos): ciclo máximo **4** (6100→2100→4100→8100→6100),
  **1968/10000** estados caen en el sumidero 0, 100 y 2500 son puntos fijos, la órbita de 1234
  tiene **63** estados, y el χ² del mapa de 10 bins es **74,18** frente a **9,78** del `msws64`
  (crítico 9 g.l. 99 % = 21,67). `msws64.h` es header-only; `ms_next_4` es `static` en
  `msws64.c` (no exportada) → replicada en el test.
- `goertzel`: `|X[k]|` es la DFT **sin normalizar** (comparable con la FFT radix-2 y con el
  analítico `N·A/2`, salvo Nyquist: `N·|cos φ|`). `goertzel_fft(re, im, n)` es **in-place** y
  devuelve 0/−1; error máximo contra la DFT directa (N=64) = **5.048e-13**. El spread entre
  formulaciones se evalúa **relativo** (`spread/mayor < 1e-8`), no en absoluto.
- Los scripts de referencia `goertzel_test.py`, `goertzel_test.m` y `goertzel_bench.py` están en la
  **raíz de `githu`**, no en el directorio del módulo; `goertzel_bench.py` dice explícitamente que
  no se ejecute para sacar conclusiones de rendimiento (Python interpretado no es comparable).
- `lcl-bench` (`/home/astrid/Documents/OpenScience/sessions/2026-10-23-0117`, repo git propio):
  los registries viven en `registries/*.json` y `mcp/bench_server.py` los carga con una **tupla
  fija** en `_load_entries()` (`attractors`, `potentials`, `energy_drift`, `goertzel_fft`,
  `archimedes_pi`, `middle_square_weyl`, `hybrid_prng`); el dispatch por `family` vive en
  `octave/registry_run.m` y `python/registry_run.py`. Constantes: `TIMEOUT_S = 180`,
  `PROTOCOL_VERSION = "2024-11-05"`, `VARIANTS = ("reference","float32","euler","euler_large","euler_single")`.
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
- Config desplegada `~/.config/Claude/claude_desktop_config.json`: **24 servidores** — el `octave` viejo
  (`/home/astrid/octave-mcp/server.py`) **volvió** (ver corrección 2026-11-13) y `md-simulation` sigue
  (`mcp-ecosystem-chiloe/server.py`, 8 tools). Backup del ciclo 6: `claude_desktop_config.json.bak-20261109-202525`;
  backup previo a la restauración: `claude_desktop_config.json.bak-20261113-205922-pre_restore_octave`.
  El template quedó en **30 servidores** (28 + `octave` restaurado).
- **Corrección 2026-11-13 (regresión del ciclo 6):** `octave` **no era duplicado** de `octave-chiloe`.
  `octave-mcp` es un **gateway** de 3 meta-tools (`list_tools`, `get_tool_schema`, `call_tool`) sobre un catálogo
  de **377 tools de dominio**; `octave-chiloe` expone 13 nativas y `md-simulation` 8. Medido con
  `inspector --cli --method tools/call --tool-name list_tools`. Los tres coexisten sin ambigüedad.
  Regla: antes de dar de baja un server por "mismo nombre/fewer tools", contar el **catálogo real**, no la
  superficie MCP; un gateway no se puede deduplicar contra un server de tools directas.
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
  **Actualizado en este ciclo**: el `octave` viejo **volvió** a la config desplegada (no era duplicado:
  gateway con 377 tools) y `md-simulation` sigue → **24 servidores**; el template quedó en **30**
  (backup previo a la restauración: `claude_desktop_config.json.bak-20261113-205922-pre_restore_octave`).
- Template: 28 → **24** claves (podadas `mcp-registry`, `inkscape`, `krita`,
  `materials-project` — binarios inexistentes — y **2 claves duplicadas** `analisis-suelos` y
  `bolsa-mcp`, cuya versión vieja pisaba a la nueva). Rutas `npx` corregidas al node real
  (`/home/astrid/.local/share/pi-node/node-v22.23.2-linux-x64/bin/npx`): `filesystem` pasó de
  caído a 14 tools y `firecrawl-mcp` de caído a 26 tools.
  **Actualizado en este ciclo**: 24 → **28** claves (`octave` → `octave-chiloe`, + `md-simulation`,
  `bench`, `mcp-blender-full`, `unrealMCP`); commit `42ab49a`. **Corrección 2026-11-13**: el renombre
  `octave` → `octave-chiloe` fue una regresión → la entrada `octave` (gateway) volvió al template: **30 claves**.
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
