# bench/ — Copia de referencia de lcl-bench

Esta carpeta contiene una **copia de referencia** (no autocontenida) de los
archivos de evaluación del repositorio `lcl-bench`. No está pensada para
ejecutarse desde `codigos-potenciados` sin los archivos complementarios que
quedan fuera de esta copia.

## Origen

- Repositorio: `lcl-bench` (`/home/astrid/Documents/OpenScience/sessions/2026-10-23-0117/`)
- Commit de origen: `931d007` ("fix: regenerar hub_compare.json y aplicar correcciones de evaluadores")
- Fecha de copia: 2026-11-05

## Qué incluye

| Archivo | Origen | Notas |
|---------|--------|-------|
| `registry_eval_hybrid_prng.m` | `lcl-bench/octave/` | Evaluador de hybrid_prng con `double()` y `uint64` explícito |
| `registry_eval_dsp.m` | `lcl-bench/octave/` | Evaluador de Goertzel/FFT (idéntico al de lcl-bench) |
| `registry_eval_middle_square_weyl.m` | `lcl-bench/octave/` | Evaluador de middle-square de von Neumann |
| `registry_eval_archimedes.m` | `lcl-bench/octave/` | Evaluador de Archimedes PI (en el benchmark se llama `archimedes_pi`) |
| `mkcheck.m`, `registry_load.m` | `lcl-bench/octave/` | Utilidades de Octave |
| `hub_compare.py` | `lcl-bench/python/` | Compara resultados entre lenguajes |
| `registry_run.py` | `lcl-bench/python/` | Ejecuta suites de evaluación |
| `translate_c.py` | `lcl-bench/python/` | Traductor de C |
| `translate_f77.py` | `lcl-bench/python/` | Traductor de Fortran 77 (incluye `_expand_labelled_do()`) |

## Qué FALTA para que sea ejecutable

Esta copia es **incompleta** por diseño. Para ejecutar `hub_compare.py` o
`registry_run.py` se necesitan además:

- `python/handles.py` — importado por `registry_run.py`
- `python/kernels.py` — importado por `registry_run.py`
- `python/diagnose_attractor.py` — importa `translate_f77.py`
- `octave/registry_run.m` — driver de Octave (no copiado)
- `registries/*.json` — definiciones de registros de evaluación
- `results/` — directorio de salida esperado por `hub_compare.py`
- Runner de Fortran para `dsp_bin` (no existe evaluador en Fortran)

## Estado actual

`bench/` sirve como **archivo de referencia** para consultar cómo están
implementados los evaluadores en `lcl-bench`. No es portable a este repositorio
sin los archivos faltantes arriba listados.

Los archivos fuente de verdad siguen viviendo en `lcl-bench`; esta copia puede
desfasarse. Para sincronizar, volver a copiar desde `lcl-bench` tras cada
cambio allí.
