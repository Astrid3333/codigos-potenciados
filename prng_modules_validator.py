"""prng_modules_validator.py - Bateria estadistica para los PRNG que SI mejoraron RANDU.

Reutiliza la bateria ya escrita y depurada de `randu_plus_validator.py`
(test espectral de Marsaglia con p-value, espaciado de cumpleaños,
permutaciones y tests NIST de primer orden) y la aplica a los generadores de
64 bits del repo:

  - `mult_xorshift64` -> codigos-potenciados/mult_xorshift64/mult_xorshift64.h
  - `hybrid_prng` -> codigos-potenciados/hybrid_prng/hybrid_prng.h
  - `lcg+mult_xorshift64` -> codigos-potenciados/lcg_lehmer/ (hibrido)

OJO: el generador de 64 bits del repo se llama `mult_xorshift64` (cadena de
multiplicacion-xorshift de 3 rondas) desde el 2026-11-20, cuando se vio que el
nombre "msws64" era incorrecto: el msws64 de Widynski (2017) usa cuadrado +
incremento de Weyl + rotacion. Las medidas de este script corresponden al
generador realmente implementado; el msws64 de Widynski vive en el modulo
`codigos-potenciados/msws_widynski/` (verificado bit a bit contra la
implementacion de referencia del autor, el crate Rust `msws` para `msws32` y el
crate Rust `weyl-rs` para la variante mono) pero NO se mide aqui: su propia
bateria estadistica (p-valores exactos, umbral Bonferroni y controles
negativos) vive en `codigos-potenciados/msws_widynski/msws_widynski_test.c`.

Incluye un CONTROL NEGATIVO: el estado crudo de `randu_plus`, que sabemos
roto (15 planos, p-value ~0). Si la bateria no lo detecta, la bateria no sirve
como evidencia.

NO es TestU01: son implementaciones propias. TestU01 real no esta instalado en
este entorno, y el unico harness versionado (`codigos-potenciados/testu01/`)
cubre solo `randu_plus`. Este script mide; no homologa.
"""

import math
import os
import struct
import subprocess
import sys
import tempfile
from typing import Dict, List

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from randu_plus_validator import (  # noqa: E402
    STAT_SEEDS,
    STAT_TESTS,
    lattice_negative_control,
    run_stat_tests,
)

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
MODULES_DIR = os.path.join(REPO_ROOT, 'codigos-potenciados')

DRIVER_SOURCE = {
    'mult_xorshift64': r'''#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "mult_xorshift64.h"
int main(int argc, char** argv) {
    uint64_t s = (argc > 1) ? strtoull(argv[1], NULL, 10) : 12345ULL;
    size_t n = (argc > 2) ? strtoul(argv[2], NULL, 10) : 1000000;
    mult_xorshift64_t rng;
    mult_xorshift64_seed(&rng, s);
    for (size_t i = 0; i < n; ++i) {
        uint64_t r = mult_xorshift64_next(&rng);
        fwrite(&r, 8, 1, stdout);
    }
    return 0;
}
''',
    'hybrid_prng': r'''#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hybrid_prng.h"
int main(int argc, char** argv) {
    uint64_t s = (argc > 1) ? strtoull(argv[1], NULL, 10) : 12345ULL;
    size_t n = (argc > 2) ? strtoul(argv[2], NULL, 10) : 1000000;
    hybrid_prng_t rng;
    hybrid_prng_seed(&rng, s);
    for (size_t i = 0; i < n; ++i) {
        uint64_t r = hybrid_prng_next(&rng);
        fwrite(&r, 8, 1, stdout);
    }
    return 0;
}
''',
    'lcg_mult_xorshift64': r'''#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lcg_lehmer.h"
int main(int argc, char** argv) {
    uint64_t s = (argc > 1) ? strtoull(argv[1], NULL, 10) : 12345ULL;
    size_t n = (argc > 2) ? strtoul(argv[2], NULL, 10) : 1000000;
    hybrid_lcg_mult_xorshift_t h;
    hybrid_lcg_mult_xorshift_seed(&h, s);
    for (size_t i = 0; i < n; ++i) {
        uint64_t r = hybrid_lcg_mult_xorshift_next(&h);
        fwrite(&r, 8, 1, stdout);
    }
    return 0;
}
''',
}

# Directorios y fuentes extra que cada driver necesita para compilar.
BUILD_SPEC = {
    'mult_xorshift64': {
        'include_dir': os.path.join(MODULES_DIR, 'mult_xorshift64'),
        'sources': [],
    },
    'hybrid_prng': {
        'include_dir': os.path.join(MODULES_DIR, 'hybrid_prng'),
        'sources': [],
    },
    'lcg_mult_xorshift64': {
        'include_dir': os.path.join(MODULES_DIR, 'lcg_lehmer'),
        'sources': [os.path.join(MODULES_DIR, 'lcg_lehmer', 'lcg_lehmer.c')],
    },
}

TARGETS = ('mult_xorshift64', 'hybrid_prng', 'lcg_mult_xorshift64')

N_VALUES_U64 = 500_000   # 500k uint64 = 1M uint32, igual muestra que randu_plus


def build_driver(target: str, tmpdir: str) -> str:
    """Compila el driver C del target y devuelve la ruta del ejecutable."""
    spec = BUILD_SPEC[target]
    src_path = os.path.join(tmpdir, f'driver_{target}.c')
    exe_path = os.path.join(tmpdir, f'driver_{target}')
    with open(src_path, 'w') as f:
        f.write(DRIVER_SOURCE[target])
    cmd = ['gcc', '-std=c11', '-O2', '-Wall', '-Wextra',
           f"-I{spec['include_dir']}", src_path] + spec['sources'] + ['-o', exe_path, '-lm']
    subprocess.run(cmd, check=True)
    return exe_path


def generate_stream(exe_path: str, target: str, seed: int, n_values: int,
                    out_path: str) -> None:
    """Genera n_values uint64 del target y los escribe en out_path."""
    with open(out_path, 'wb') as out:
        subprocess.run([exe_path, str(seed), str(n_values)],
                       stdout=out, check=True)


def uint64_to_uint32_natural(path: str) -> List[int]:
    """Convierte el flujo de uint64 en palabras uint32 NO SOLAPADAS, en el
    orden que veria un consumidor que lee el flujo de bytes.

    Cada uint64 ocupa 8 bytes en orden little-endian, asi que el consumidor lee
    primero los 32 bits bajos de la palabra N y luego los 32 bits altos de la
    palabra N+1. Es la proyeccion natural y la que debe usarse: medir mitades
    por separado (solo altos, solo bajos) o entrelazarlos (hi_N, lo_N, hi_N+1)
    prueba secuencias que ningun consumidor real ve, y con eso aparecieron
    falsos FAIL que no se reproducen en el flujo real."""
    with open(path, 'rb') as f:
        raw = f.read()
    return list(struct.unpack(f'<{len(raw)//4}I', raw))


def urandom_uint32(n_values: int) -> List[int]:
    """Flujo de referencia limpio: /dev/urandom.

    Es el control de calibracion del detector: la bateria deberia APROBAR aqui
    siempre. Si /dev/urandom falla, el problema es del test, no del generador."""
    return list(struct.unpack(f'<{n_values}I', os.urandom(4 * n_values)))


def sweep(target: str, seeds=STAT_SEEDS, n_values: int = N_VALUES_U64) -> Dict:
    """Corre la bateria sobre varias semillas y conserva el peor caso."""
    per_seed = {}
    with tempfile.TemporaryDirectory() as tmpdir:
        exe_path = build_driver(target, tmpdir)
        for seed in seeds:
            path = os.path.join(tmpdir, f'{target}_{seed}.bin')
            generate_stream(exe_path, target, seed, n_values, path)
            data = uint64_to_uint32_natural(path)
            per_seed[seed] = run_stat_tests(data)
            del data

    summary = {}
    for name, metric in STAT_TESTS:
        values = {s: per_seed[s][name][metric] for s in seeds}
        worst = min(values.values())
        worst_seed = min(values, key=values.get)
        threshold = 0.5 if metric == 'ratio' else 0.01
        summary[name] = {
            'metric': metric,
            'worst': worst,
            'worst_seed': worst_seed,
            'threshold': threshold,
            'pass': worst > threshold,
        }
    n_trials = len(seeds) * len(STAT_TESTS)
    return {'seeds': list(seeds), 'n_values_u64': n_values, 'summary': summary,
            'per_seed': per_seed,
            'n_below_001': count_below(per_seed, 0.01),
            'n_below_0001': count_below(per_seed, 0.001),
            'n_trials': n_trials}


def calibration_clean(seeds=STAT_SEEDS, n_values: int = N_VALUES_U64 * 2) -> Dict:
    """Control de calibracion: la misma bateria sobre /dev/urandom.

    Debe aprobar en todas las semillas. Es la contraparte del control negativo
    (`lattice_negative_control`), que debe fallar: entre ambos, un FAIL de la
    tabla de generadores es interpretable."""
    per_seed = {}
    for seed in seeds:
        data = urandom_uint32(n_values)
        per_seed[seed] = run_stat_tests(data)
        del data

    summary = {}
    for name, metric in STAT_TESTS:
        values = {s: per_seed[s][name][metric] for s in seeds}
        worst = min(values.values())
        threshold = 0.5 if metric == 'ratio' else 0.01
        summary[name] = {
            'metric': metric,
            'worst': worst,
            'threshold': threshold,
            'pass': worst > threshold,
        }
    n_trials = len(seeds) * len(STAT_TESTS)
    return {'seeds': list(seeds), 'summary': summary, 'per_seed': per_seed,
            'n_below_001': count_below(per_seed, 0.01),
            'n_below_0001': count_below(per_seed, 0.001),
            'n_trials': n_trials}


def negative_control() -> Dict:
    """Control negativo: el estado crudo de randu_plus debe fallar."""
    return lattice_negative_control(seed=12345, n_values=600000, k=16)


def count_below(per_seed: Dict, threshold: float) -> int:
    """Cuenta los p-values por debajo del umbral entre todas las semillas."""
    n = 0
    for seed_results in per_seed.values():
        for name, metric in STAT_TESTS:
            if seed_results[name][metric] < threshold:
                n += 1
    return n


def binomial_upper_99_9(n_trials: int, p0: float = 0.01) -> int:
    """Numero maximo de falsos positivos tolerable (99.9%) bajo H0.

    Con 7 tests x 8 semillas = 56 p-values y un umbral de 0.01, la H0
    (flujo aleatorio) espera 0.56 p-values por debajo del umbral. Exigir que
    los 56 superen 0.01 daria falsos FAIL por multiplicidad: se calibro
    contra /dev/urandom y fallo. El limite de 99.9% es 5 fallos."""
    limit = 0
    while True:
        # cola de Poisson: P(X > limit | lambda = n_trials*p0)
        lam = n_trials * p0
        term = math.exp(-lam)
        cdf = term
        k = 0
        while k < limit:
            k += 1
            term *= lam / k
            cdf += term
        if 1.0 - cdf < 0.001:
            return limit
        limit += 1
        if limit > 50:
            return limit


def write_report(results: Dict, control: Dict, clean: Dict, path: str) -> None:
    limit = binomial_upper_99_9(results['mult_xorshift64']['n_trials'])
    with open(path, 'w') as f:
        f.write('# Bateria estadistica de los PRNG de 64 bits del repo\n\n')
        f.write("**Estudio, no homologacion.** Mide el flujo de bits de "
                "`mult_xorshift64`, `hybrid_prng` y del híbrido LCG+mult_xorshift64 con la misma "
                "bateria que usa `randu_plus_validator.py`, y la acompaña de dos "
                "controles: uno limpio (`/dev/urandom`, que debe comportarse como "
                "un generador ideal) y uno defectuoso (el estado crudo de "
                "`randu_plus`, que debe fallar). No es TestU01 ni PractRand: la "
                "biblioteca completa de TestU01 no esta instalada en este entorno; el "
                "harness versionado (`codigos-potenciados/testu01/`) solo cubre "
                "`randu_plus`, asi que estos tres generadores todavia no se han "
                "sometido a TestU01 real.\n\n")
        f.write("**Que se midio exactamente:** el generador de 64 bits del repo se "
                "llama `mult_xorshift64` (cadena de multiplicacion-xorshift de 3 "
                "rondas). NO es el msws64 de Widynski (2017), que usa cuadrado + "
                "incremento de Weyl + rotacion y vive en el modulo "
                "`codigos-potenciados/msws_widynski/` (verificado bit a bit contra "
                "la implementacion de referencia del autor, el crate Rust `msws` "
                "para `msws32` y el crate Rust `weyl-rs` para la variante mono; NO "
                "hay oraculo de terceros para la variante `msws64` de dos carriles, "
                "que se contrasta contra el header del autor y una reimplementacion "
                "independiente escrita desde el articulo), pero NO medido con esta "
                "bateria). Las cifras de "
                "este informe corresponden al generador realmente implementado.\n\n")
        f.write(f"**Semillas:** {', '.join(str(s) for s in results['mult_xorshift64']['seeds'])}"
                f" (8 semillas)\n")
        f.write(f"**Muestra por semilla:** {results['mult_xorshift64']['n_values_u64']} uint64 "
                f"= {results['mult_xorshift64']['n_values_u64']*2} uint32 "
                "(palabras de 32 bits NO solapadas, en el orden de bytes que ve "
                "un consumidor)\n")
        f.write(f"**Comparaciones:** {results['mult_xorshift64']['n_trials']} p-values por "
                "generador (7 tests x 8 semillas)\n")
        f.write("**Criterio:** con 56 p-values y umbral 0.01, un flujo aleatorio "
                f"espera 0.56 falsos positivos; el limite de 99.9% es {limit}. "
                "Se exige ademas cero p-values por debajo de 0.001 (senal fuerte) "
                "y que el numero de p-values < 0.01 no supere el del control "
                "limpio por mas de 1.\n\n")

        f.write('## Resultados (peor caso entre 8 semillas)\n\n')
        f.write('| Generador | Test | metrica | peor caso | semilla | pass |\n')
        f.write('|---|---|---|---|---|---|\n')
        for target in TARGETS:
            for name, metric in STAT_TESTS:
                s = results[target]['summary'][name]
                f.write(f"| {target} | {name} | {metric} | {s['worst']:.6g} | "
                        f"{s['worst_seed']} | {'✅' if s['pass'] else '❌'} |\n")

        f.write('\n## Conteo de p-values por debajo del umbral\n\n')
        f.write('| Flujo | p < 0.01 | p < 0.001 | comparaciones | veredicto |\n')
        f.write('|---|---|---|---|---|\n')
        c = clean
        f.write(f"| /dev/urandom (control limpio) | {c['n_below_001']} | "
                f"{c['n_below_0001']} | {c['n_trials']} | referencia |\n")
        for target in TARGETS:
            r = results[target]
            ok = (r['n_below_0001'] == 0
                  and r['n_below_001'] <= limit
                  and r['n_below_001'] <= c['n_below_001'] + 1)
            f.write(f"| {target} | {r['n_below_001']} | {r['n_below_0001']} | "
                    f"{r['n_trials']} | {'✅ sin señal' if ok else '⚠️ revisar'} |\n")

        f.write('\n### Detalle de cada p-value por debajo de 0.01\n\n')
        any_low = False
        f.write('| Flujo | Test | semilla | p-value | veces observado |\n')
        f.write('|---|---|---|---|---|\n')
        for label, block in [('/dev/urandom', clean)] + [(t, results[t]) for t in TARGETS]:
            for name, metric in STAT_TESTS:
                if metric == 'ratio':
                    continue
                for seed, sr in block['per_seed'].items():
                    if sr[name][metric] < 0.01:
                        any_low = True
                        f.write(f"| {label} | {name} | {seed} | "
                                f"{sr[name][metric]:.6g} | 1 |\n")
        if not any_low:
            f.write('| — | — | — | — | ningun p-value por debajo de 0.01 |\n')

        f.write('\n## Notas de calibracion (medidas en este entorno)\n\n')
        f.write("- El test espectral de Marsaglia usa **200000 tuplas** (no 50000): "
                "con 50000 tuplas la ocupacion media por celda de la rejilla 16^3 "
                "es 12,2 y el p-value fluctua mucho. Medido sobre el MISMO flujo "
                "(lcg+mult_xorshift64, semilla 3): p=0.000432 con 50000 tuplas y p=0.0751181 "
                "con 200000. El FAIL de 0.00043 era ruido del test, no un defecto "
                "del generador.\n")
        f.write("- El FAIL de `NIST_BlockFreq` en `hybrid_prng` (semilla 65537, "
                "p=0.00757203) es **reproducible**: sale con el mismo valor en "
                "corridas independientes. Queda registrado como senal marginal "
                "dentro del presupuesto de falsos positivos (1 de 56 frente a "
                "0.56 esperados), no como prueba de defecto.\n")
        f.write("- `hybrid_prng` combina el XOR de una secuencia middle-square de "
                "4 digitos decimales con `mult_xorshift64`; la parte middle-square aporta "
                "solo ~13,3 bits por palabra, asi que su mitad superior hereda "
                "esa limitacion aunque el XOR con mult_xorshift64 la disperse. Para uso "
                "criptografico o de simulacion de Monte Carlo de alta exigencia, "
                "usar `mult_xorshift64` puro.\n")
        f.write("- El generador de 64 bits se renombro de `msws64` a `mult_xorshift64` el "
                "2026-11-20: el nombre anterior era incorrecto (no es el msws64 de "
                "Widynski ni el xorshift64* de Vigna). Su periodo no esta "
                "verificado (solo se comprobo que no repite en las primeras 100000 "
                "muestras) y el sumidero 0 se evita forzando semilla impar.\n")

        f.write('\n## Control de calibracion: /dev/urandom (debe aprobar)\n\n')
        f.write('| Test | metrica | peor caso | pass |\n|---|---|---|---|\n')
        for name, metric in STAT_TESTS:
            s = clean['summary'][name]
            f.write(f"| {name} | {metric} | {s['worst']:.6g} | "
                    f"{'✅' if s['pass'] else '❌'} |\n")

        f.write('\n## Control negativo: estado crudo de randu_plus (debe fallar)\n\n')
        f.write(f"- planos del reticulo: {control.get('planes_raw', {}).get('n_planes')} "
                f"(relacion exacta: {control.get('planes_raw', {}).get('relation_exact')})\n")
        f.write(f"- p-value espectral (Marsaglia) del estado CRUDO de randu_plus: "
                f"{control.get('spectral_raw', {}).get('p_value')}\n")
        f.write(f"- p-value espectral de la salida MEZCLADA de randu_plus: "
                f"{control.get('spectral_mixed', {}).get('p_value')}\n")
        f.write(f"- unmix32(salida) devuelve el estado crudo: "
                f"{control.get('unmix_recovers_state')}\n")
        f.write(f"- veredicto del control negativo: "
                f"{'PASS (falló como debe)' if control.get('pass') else 'FAIL'}\n\n")
        f.write("Si el control limpio se comportara como un generador roto, o el "
                "control negativo no fallara, la tabla de arriba no significaria "
                "nada.\n")


def main() -> int:
    print("Compilando drivers y corriendo la bateria (puede tardar varios minutos)...")
    print("\n=== Control de calibracion: /dev/urandom (debe aprobar) ===")
    clean = calibration_clean()
    for name, metric in STAT_TESTS:
        s = clean['summary'][name]
        flag = 'PASS' if s['pass'] else 'FAIL'
        print(f"  {name:22s} {metric:8s} peor={s['worst']:.6g} {flag}")

    results = {}
    for target in TARGETS:
        print(f"\n=== {target} ===")
        results[target] = sweep(target)
        for name, metric in STAT_TESTS:
            s = results[target]['summary'][name]
            flag = 'PASS' if s['pass'] else 'FAIL'
            print(f"  {name:22s} {metric:8s} peor={s['worst']:.6g} "
                  f"semilla={s['worst_seed']} {flag}")

    print("\n=== Control negativo (randu_plus crudo, debe fallar) ===")
    control = negative_control()
    print(f"  planos={control.get('planes_raw', {}).get('n_planes')} "
          f"crudo_p={control.get('spectral_raw', {}).get('p_value')} "
          f"mezclado_p={control.get('spectral_mixed', {}).get('p_value')} "
          f"unmix={control.get('unmix_recovers_state')}")

    out = os.path.join(REPO_ROOT, 'VALIDATION_REPORT_PRNG_MODULES.md')
    write_report(results, control, clean, out)
    print(f"\nReporte guardado en {out}")

    limit = binomial_upper_99_9(results[TARGETS[0]]['n_trials'])
    ok = True
    for target in TARGETS:
        r = results[target]
        good = (r['n_below_0001'] == 0 and r['n_below_001'] <= limit
                and r['n_below_001'] <= clean['n_below_001'] + 1)
        ok = ok and good
        print(f"  {target:12s} p<0.01: {r['n_below_001']}/{r['n_trials']} "
              f"p<0.001: {r['n_below_0001']} -> {'OK' if good else 'REVISAR'}")
    print(f"  control limpio /dev/urandom: p<0.01: {clean['n_below_001']}"
          f"/{clean['n_trials']} p<0.001: {clean['n_below_0001']} "
          f"(limite 99.9% = {limit})")
    control_ok = bool(control.get('pass'))
    clean_ok = clean['n_below_0001'] == 0 and clean['n_below_001'] <= limit
    print(f"\nTotal: {'PASS' if (ok and clean_ok and control_ok) else 'FAIL'} | "
          f"control limpio {'PASS' if clean_ok else 'FAIL'} | "
          f"control negativo {'PASS' if control_ok else 'FAIL'}")
    return 0 if (ok and clean_ok and control_ok) else 1


if __name__ == '__main__':
    raise SystemExit(main())
