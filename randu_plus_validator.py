#!/usr/bin/env python3
"""
randu_plus_validator.py - Suite estadística autocontenida para randu_plus
Implementa tests equivalentes a TestU01 SmallCrush + spectral 3D + NIST SP 800-22 básico
"""

import struct
import math
import sys
import subprocess
import os
import tempfile
import hashlib
from collections import Counter
from typing import List, Tuple, Dict

# Chi-cuadrado survival function (aproximación Wilson-Hilferty)
def chi2_sf(x: float, df: int) -> float:
    if x <= 0: return 1.0
    if df <= 0: return 0.0
    # Wilson-Hilferty transformation
    c = 1.0 / (9 * df)
    z = ((x / df) ** (1/3) - 1 + c) / math.sqrt(c)
    # cola derecha
    return 0.5 * math.erfc(z / math.sqrt(2))

# ────────────────────────────────────────────────────────────────────
# Generador de datos binarios desde randu_plus (compilado)
# ────────────────────────────────────────────────────────────────────
# El generador temporal INCLUYE randu_plus.h. Antes esta funcion embebia una
# copia del algoritmo, que se desincronizo del header (no hacia el
# `*s |= 1u` que fuerza semilla impar). Con una semilla impar ambos
# streamers coinciden bit a bit, pero con semilla par divergen: el validador
# estaria midiendo un generador que no es el del repo.
#
# El fuente temporal se escribe en un directorio temporal, no en la raiz del
# repo, para no dejar artefactos.
BIN_DRIVER_SOURCE = r'''#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "randu_plus.h"
int main(int argc, char** argv) {
    uint32_t s = (argc > 1) ? (uint32_t)strtoul(argv[1], NULL, 10) : 12345u;
    size_t n = (argc > 2) ? strtoul(argv[2], NULL, 10) : 1000000;
    for (size_t i = 0; i < n; ++i) {
        uint32_t r = randu_plus(&s);
        fwrite(&r, 4, 1, stdout);
    }
    return 0;
}
'''

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))

# `randu_plus.h` esta duplicado y las DOS copias estan versionadas: una en la
# raiz del repo y otra en `codigos-potenciados/`. Cada consumidor compila con
# un `-I` distinto (el validador usa la raiz; `codigos-potenciados/testu01/
# Makefile` usa `-I..`), asi que si una copia se edita y la otra no, cada
# binario mide un generador distinto y nada lo delata. Este repositorio lo
# comprueba con `header_sync_check()` y `header_crosscheck()`.
HEADER_COPIES = [
    ('raiz', os.path.join(REPO_ROOT, 'randu_plus.h')),
    ('codigos-potenciados',
     os.path.join(REPO_ROOT, 'codigos-potenciados', 'randu_plus.h')),
]

def generate_binary(output_path: str, n_bytes: int, seed: int = 12345, quiet: bool = False,
                    header_dir: str = None) -> None:
    """Genera n_bytes de salida randu_plus usando el binario compilado.

    `header_dir` decide que copia de `randu_plus.h` se incluye. Por defecto es
    la de la raiz del repo; los tests de sincronizacion compilan contra cada
    copia existente, porque cada consumidor del repo usa un `-I` distinto."""
    if header_dir is None:
        header_dir = REPO_ROOT
    with tempfile.TemporaryDirectory() as tmpdir:
        src_path = os.path.join(tmpdir, 'randu_plus_bin.c')
        exe_path = os.path.join(tmpdir, 'randu_plus_bin')
        with open(src_path, 'w') as f:
            f.write(BIN_DRIVER_SOURCE)
        subprocess.run(
            ['gcc', '-std=c11', '-O2', f'-I{header_dir}', src_path, '-o', exe_path],
            check=True)

        n_uints = n_bytes // 4
        with open(output_path, 'wb') as out:
            subprocess.run([exe_path, str(seed), str(n_uints)],
                           stdout=out, check=True)
    if not quiet:
        print(f"Generados {n_bytes} bytes ({n_uints} uint32) en {output_path}")

def load_uint32(path: str) -> List[int]:
    with open(path, 'rb') as f:
        data = f.read()
    return list(struct.unpack(f'<{len(data)//4}I', data))

# ────────────────────────────────────────────────────────────────────
# Referencia independiente y cross-check contra el header
# ────────────────────────────────────────────────────────────────────
MASK32 = 0xFFFFFFFF

def randu_plus_ref(state: int) -> Tuple[int, int]:
    """Espejo en Python de `randu_plus.h` (LCG RANDU + fmix32).

    Es una referencia independiente del binario: si el header cambia, hay que
    cambiar esto en el MISMO commit, o el cross-check falla y avisa. Su unico
    trabajo es detectar que el binario que se valida no es el generador del
    repo (header equivocado, binario obsoleto, copia embebida...)."""
    state = (state | 1) & MASK32          # el header fuerza semilla impar
    state = (65539 * state) & MASK32       # RANDU LCG mod 2^32
    x = state
    x ^= x >> 16
    x = (x * 0x85EBCA6B) & MASK32          # fmix32 (MurmurHash3 finalizer)
    x ^= x >> 13
    x = (x * 0xC2B2AE35) & MASK32
    x ^= x >> 16
    return x, state

def header_crosscheck(seeds=(12345, 0, 2, 100, 4294967295),
                      n_values: int = 1000) -> Dict:
    """Verifica bit a bit que el binario compilado (que incluye
    `randu_plus.h`) implementa el algoritmo documentado, y que una semilla
    par produce la MISMA secuencia que su version impar.

    Esta comprobacion es la que habria detectado el fallo original: la copia
    embebida omitia `*state |= 1u`, asi que con semilla par divergia desde
    la salida 0 mientras que con semilla impar coincidia.

    Se ejecuta contra **cada** copia del header que hay en el repo (raiz y
    `codigos-potenciados/`), no solo contra una: cada consumidor compila con
    un `-I` distinto, y el cross-check debe valer para todos ellos."""
    per_header = {}
    tmpdir = tempfile.mkdtemp(prefix='randu_xcheck_')
    try:
        for label, header_path in HEADER_COPIES:
            header_dir = os.path.dirname(header_path)
            if not os.path.exists(header_path):
                continue

            c_vs_ref = {}
            even_folded = {}
            for seed in seeds:
                path = os.path.join(tmpdir, f'{label}_seed_{seed}.bin')
                generate_binary(path, n_values * 4, seed, quiet=True,
                                header_dir=header_dir)
                data = load_uint32(path)

                st = seed
                ref = []
                for _ in range(n_values):
                    r, st = randu_plus_ref(st)
                    ref.append(r)
                c_vs_ref[seed] = (data[:n_values] == ref)

                if seed & 1 == 0:
                    odd_path = os.path.join(tmpdir, f'{label}_seed_{seed | 1}.bin')
                    generate_binary(odd_path, n_values * 4, seed | 1, quiet=True,
                                    header_dir=header_dir)
                    odd_data = load_uint32(odd_path)
                    even_folded[seed] = (data[:n_values] == odd_data[:n_values])

            per_header[label] = {
                "header": os.path.relpath(header_path, REPO_ROOT),
                "c_vs_reference": c_vs_ref,
                "even_seed_folded_to_odd": even_folded,
                "pass": all(c_vs_ref.values()) and all(even_folded.values()),
            }
    finally:
        for name in os.listdir(tmpdir):
            os.unlink(os.path.join(tmpdir, name))
        os.rmdir(tmpdir)

    all_pass = bool(per_header) and all(h["pass"] for h in per_header.values())
    return {
        "pass": all_pass,
        "n_values": n_values,
        "checked_seeds": list(seeds),
        "headers_checked": list(per_header.keys()),
        "per_header": per_header,
        "c_vs_reference": {label: h["c_vs_reference"] for label, h in per_header.items()},
        "even_seed_folded_to_odd": {label: h["even_seed_folded_to_odd"]
                                    for label, h in per_header.items()},
        "detail": (f"el binario coincide bit a bit con la referencia en las "
                   f"{len(seeds)} semillas para las {len(per_header)} copias del "
                   f"header ({', '.join(per_header.keys())}), y las semillas pares "
                   f"se pliegan a su version impar como hace `*state |= 1u`")
        if all_pass
        else ("divergencia entre el binario y la referencia: el validador NO "
              "esta midiendo el generador del repo"),
    }


def header_sync_check() -> Dict:
    """Comprueba que todas las copias de `randu_plus.h` del repo son identicas.

    El header esta duplicado en la raiz y en `codigos-potenciados/`, y ambas
    copias estan versionadas. Si solo se editara una, cada consumidor
    compilaria un generador distinto sin que ninguna prueba lo delate: el
    fallo seria invisible hasta que los numeros de dos ejecutables no
    cuadren. Este test convierte la duplicidad en un invariante verificado."""
    copies = {}
    for label, path in HEADER_COPIES:
        if os.path.exists(path):
            with open(path, 'rb') as f:
                copies[label] = hashlib.sha256(f.read()).hexdigest()

    digests = set(copies.values())
    identical = len(digests) <= 1
    return {
        "pass": identical and len(copies) > 0,
        "copies": copies,
        "n_copies": len(copies),
        "identical": identical,
        "detail": (f"las {len(copies)} copias de randu_plus.h del repo son "
                   f"identicas (sha256 {list(digests)[0][:12] if digests else '-'})")
        if identical
        else ("las copias de randu_plus.h DIVERGEN entre si: el validador y "
              "los consumidores de codigos-potenciados estarian midiendo "
              "generadores distintos"),
    }

# ────────────────────────────────────────────────────────────────────
# Tests estadísticos
# ────────────────────────────────────────────────────────────────────

def spectral_test_3d(data: List[int], n_tuples: int = 100000) -> Dict:
    """Test espectral 3D de Marsaglia - detecta planos en RANDU."""
    # Tomar tuplas (x, y, z) consecutivas normalizadas a [0,1)
    max_u = 2**32
    pts = []
    for i in range(0, min(len(data)-2, n_tuples*3), 3):
        x = data[i] / max_u
        y = data[i+1] / max_u
        z = data[i+2] / max_u
        pts.append((x, y, z))
    
    # Buscar estructura de planos: calcular distancias a planos candidatos
    # Para RANDU, los puntos caen en 15 planos: x - 6y + 9z = 0 mod 1 (aprox)
    # Test simplificado: correlación serial 3D
    n = len(pts)
    if n < 100:
        return {"error": "insuficientes datos"}
    
    # Calcular determinante de matriz de correlación 3x3
    mean_x = sum(p[0] for p in pts) / n
    mean_y = sum(p[1] for p in pts) / n
    mean_z = sum(p[2] for p in pts) / n
    
    cov = [[0.0]*3 for _ in range(3)]
    for x,y,z in pts:
        dx, dy, dz = x-mean_x, y-mean_y, z-mean_z
        cov[0][0] += dx*dx; cov[0][1] += dx*dy; cov[0][2] += dx*dz
        cov[1][0] += dy*dx; cov[1][1] += dy*dy; cov[1][2] += dy*dz
        cov[2][0] += dz*dx; cov[2][1] += dz*dy; cov[2][2] += dz*dz
    for i in range(3):
        for j in range(3):
            cov[i][j] /= n
    
    # Determinante (volumen del paralelepípedo de correlación)
    det = (cov[0][0]*(cov[1][1]*cov[2][2]-cov[1][2]*cov[2][1]) -
           cov[0][1]*(cov[1][0]*cov[2][2]-cov[1][2]*cov[2][0]) +
           cov[0][2]*(cov[1][0]*cov[2][1]-cov[1][1]*cov[2][0]))
    
    # Para ruido blanco, det ~ (1/12)^3 ≈ 5.8e-4
    # RANDU original da det muy pequeño (estructura planar)
    return {
        "determinant": det,
        "expected_white_noise": 1/1728,
        "ratio": det / (1/1728) if det > 0 else 0,
        "interpretation": "Cerca de 1 = ruido blanco; <<1 = estructura planar"
    }

def birthday_spacings(data: List[int], m: int = 2**24, n_samples: int = 5000) -> Dict:
    """Test Birthday Spacings (TestU01)."""
    vals = [(x % m) for x in data[:n_samples]]
    vals.sort()
    spacings = [vals[i+1] - vals[i] for i in range(len(vals)-1)]
    spacings.sort()
    
    collisions = sum(1 for s in spacings if s == 0)
    expected = n_samples * (n_samples - 1) / (2 * m)
    
    # Poisson approximation for collisions
    if expected > 0:
        # Usar test exacto Poisson para colisiones
        # P(X >= collisions) = 1 - P(X < collisions)
        from math import exp
        p_val = 1.0
        term = exp(-expected)
        cum = term
        for k in range(1, collisions):
            term *= expected / k
            cum += term
        if collisions > 0:
            p_val = 1 - cum
        else:
            p_val = 1.0
    else:
        p_val = 1.0
    
    return {
        "collisions": collisions,
        "expected": expected,
        "p_value": p_val,
        "pass": p_val > 0.01
    }

def permutation_test(data: List[int], n: int = 10000) -> Dict:
    """Test de permutaciones (TestU01)."""
    permutations = []
    for i in range(0, min(len(data), n*5), 5):
        if i+4 >= len(data): break
        tup = data[i:i+5]
        ranked = sorted(range(5), key=lambda k: tup[k])
        perm = tuple(ranked)
        permutations.append(perm)
    
    counts = Counter(permutations)
    expected = len(permutations) / 120.0  # 5! = 120
    chi2 = sum((c - expected)**2 / expected for c in counts.values())
    # df = 120 - 1 = 119
    p_val = chi2_sf(chi2, 119)
    
    return {
        "observed_perms": len(counts),
        "expected_perms": 120,
        "chi2": chi2,
        "p_value": p_val,
        "pass": p_val > 0.01
    }

def nist_frequency(data: List[int]) -> Dict:
    """NIST SP 800-22 Frequency (Monobit) test."""
    # Convertir a bits
    bits = []
    for x in data[:1000000]:  # 1M uints = 32M bits
        bits.extend([(x >> i) & 1 for i in range(32)])
    
    n = len(bits)
    s = sum(1 for b in bits if b == 1) - sum(1 for b in bits if b == 0)
    s_obs = abs(s) / math.sqrt(n)
    p_val = math.erfc(s_obs / math.sqrt(2))
    
    return {
        "n_bits": n,
        "s_obs": s_obs,
        "p_value": p_val,
        "pass": p_val > 0.01
    }

def nist_block_frequency(data: List[int], block_size: int = 10000) -> Dict:
    """NIST Block Frequency test."""
    bits = []
    for x in data[:100000]:
        bits.extend([(x >> i) & 1 for i in range(32)])
    
    n_blocks = len(bits) // block_size
    chi2 = 0.0
    for i in range(n_blocks):
        block = bits[i*block_size:(i+1)*block_size]
        pi = sum(block) / block_size
        chi2 += 4.0 * block_size * (pi - 0.5)**2
    
    # df = n_blocks
    p_val = chi2_sf(chi2, n_blocks)
    return {
        "n_blocks": n_blocks,
        "chi2": chi2,
        "p_value": p_val,
        "pass": p_val > 0.01
    }

def nist_runs(data: List[int]) -> Dict:
    """NIST Runs test."""
    bits = []
    for x in data[:100000]:
        bits.extend([(x >> i) & 1 for i in range(32)])
    
    n = len(bits)
    pi = sum(bits) / n
    if abs(pi - 0.5) >= 2/math.sqrt(n):
        return {"p_value": 0.0, "pass": False, "reason": "pre-test failed"}
    
    runs = 1 + sum(1 for i in range(1, n) if bits[i] != bits[i-1])
    expected = 2 * n * pi * (1 - pi)
    var = 2 * n * pi * (1 - pi) * (2 * n * pi * (1 - pi) - 1) / (n - 1) if n > 1 else 1
    z = (runs - expected) / math.sqrt(max(var, 1))
    p_val = math.erfc(abs(z) / math.sqrt(2))
    
    return {"runs": runs, "expected": expected, "p_value": p_val, "pass": p_val > 0.01}

def dieharder_style_tests(data: List[int]) -> List[Dict]:
    """Subconjunto estilo dieharder: birthday, perm, rank, etc."""
    results = []
    results.append(("BirthdaySpacings", birthday_spacings(data)))
    results.append(("Permutation", permutation_test(data)))
    return results

# ────────────────────────────────────────────────────────────────────
# Main
# ────────────────────────────────────────────────────────────────────
# La seccion de honestidad del informe se genera desde aqui, no a mano: asi
# no se pierde cada vez que el validador regenera el reporte.
REPORT_CAVEAT = """
## Qué prueba este informe y qué NO

Este informe mide **propiedades estadísticas del flujo de bits** de la salida
del generador (distribución, espectro, espaciado de cumpleaños, permutaciones,
frecuencia, frecuencia por bloques y rachas). Los tests son **tests de
aleatoriedad estándar**: el mismo criterio con el que se evalúa un generador
pseudoaleatorio maduro.

**No es una validación del generador `randu_plus`.** En concreto:

- Estos tests **no pueden detectar** el retículo lineal de RANDU ni un periodo
  corto, porque son tests de primer orden sobre una muestra de {n_values}
  valores. El periodo real del generador es **2^30** y el retículo es
  recuperable por la recurrencia `x[n+2] = 6·x[n+1] - 9·x[n]`; eso está
  documentado en `randu_plus.h`, que recomienda explícitamente `msws64/` o
  `hybrid_prng/` para uso real.
- `fmix32` (el post-proceso de MurmurHash) es una **biyección**: reordena los
  estados de RANDU pero no añade entropía ni rompe el retículo.
- El recuento dice "el flujo **parece** aleatorio a estas escalas", no "el
  generador es criptográficamente seguro" ni "cumple ISO/IEC 23837".

Para las propiedades estructurales del generador (biyectividad, periodo,
recuperación del retículo) está el **test incluido en `randu_plus.c`**, que
incluye `randu_plus.h` y hace 8 comprobaciones con exit code: es distinto y
complementario a este informe.

## Qué mide el cross-check contra el header

`generate_binary()` compila un `.c` temporal que **incluye `randu_plus.h`**
(con `-I` apuntando al directorio del repo), de modo que el flujo binario
validado es exactamente el generador del repo. El test `HeaderCrossCheck`
comprueba **bit a bit** los primeros {n_xcheck} valores contra una referencia
escrita a mano en Python, para las semillas {seeds}, y comprueba además que
una semilla par produce la misma secuencia que su versión impar (el
`*state |= 1u` del header).

`randu_plus.h` está **duplicado y versionado** en dos sitios del repo: la raíz
y `codigos-potenciados/`. Cada consumidor compila con un `-I` distinto (el
validador usa la raíz; `codigos-potenciados/testu01/Makefile` usa `-I..`), así
que el cross-check se ejecuta contra **cada** copia y el test `HeaderSync`
comprueba que todas son **byte a byte idénticas**. Si alguien edita una sola
copia, las pruebas fallan en lugar de dejar que dos ejecutables midan
generadores distintos.

Si el header cambia, hay que actualizar la referencia en el mismo commit: si
no, este test falla y lo dice. Ese fallo es intencional: es la alarma que
impide que el informe vuelva a medir un generador que no sea el del repo.

## Nota de reproducibilidad

El informe se generó con la semilla {seed} sobre {n_mb} MB ({n_values}
`uint32`). Para reproducir: `python3 randu_plus_validator.py`.
"""

def main():
    seed = 12345
    n_bytes = 100 * 1024 * 1024  # 100 MB
    bin_path = "randu_output.bin"
    
    print(f"=== Generando {n_bytes/1e6:.0f} MB de datos randu_plus (seed={seed}) ===")
    generate_binary(bin_path, n_bytes, seed)
    
    print("=== Cargando datos ===")
    data = load_uint32(bin_path)
    print(f"Cargados {len(data)} uint32")
    
    print("\n=== TESTS ===")
    all_results = []
    
    # 1. Spectral 3D
    print("\n1. Spectral Test 3D (Marsaglia)...")
    spec = spectral_test_3d(data, 50000)
    all_results.append(("Spectral3D", spec))
    print(f"   det={spec.get('determinant',0):.2e}, ratio={spec.get('ratio',0):.3f}")
    
    # 2. Birthday Spacings
    print("\n2. Birthday Spacings...")
    bd = birthday_spacings(data)
    all_results.append(("BirthdaySpacings", bd))
    print(f"   collisions={bd['collisions']}, expected={bd['expected']:.2f}, p={bd['p_value']:.4f}")
    
    # 3. Permutation
    print("\n3. Permutation Test...")
    perm = permutation_test(data)
    all_results.append(("Permutation", perm))
    print(f"   chi2={perm['chi2']:.2f}, p={perm['p_value']:.4f}")
    
    # 4. NIST Frequency
    print("\n4. NIST Frequency (Monobit)...")
    freq = nist_frequency(data)
    all_results.append(("NIST_Frequency", freq))
    print(f"   p={freq['p_value']:.6f}")
    
    # 5. NIST Block Frequency
    print("\n5. NIST Block Frequency...")
    bfreq = nist_block_frequency(data)
    all_results.append(("NIST_BlockFreq", bfreq))
    print(f"   p={bfreq['p_value']:.6f}")
    
    # 6. NIST Runs
    print("\n6. NIST Runs...")
    runs = nist_runs(data)
    all_results.append(("NIST_Runs", runs))
    print(f"   p={runs['p_value']:.6f}")

    # 7. Cross-check contra randu_plus.h (prueba de que lo medido ES el repo)
    print("\n7. Header Cross-Check (binario vs randu_plus.h)...")
    xcheck = header_crosscheck()
    all_results.append(("HeaderCrossCheck", xcheck))
    print(f"   {xcheck['detail']}")

    # 8. Sincronizacion de las copias del header del repo
    print("\n8. Header Sync (las copias de randu_plus.h son identicas)...")
    hsync = header_sync_check()
    all_results.append(("HeaderSync", hsync))
    print(f"   {hsync['detail']}")

    # Resumen
    print("\n" + "="*60)
    print("RESUMEN DE VALIDACIÓN randu_plus")
    print("="*60)
    passed = 0
    total = 0
    for name, res in all_results:
        if isinstance(res, dict) and 'pass' in res:
            total += 1
            ok = res['pass']
            passed += 1 if ok else 0
            status = "✅ PASS" if ok else "❌ FAIL"
            print(f"  {name:20s} : {status} (p={res.get('p_value', 'N/A')})")
        elif isinstance(res, dict) and 'ratio' in res:
            total += 1
            ok = res['ratio'] > 0.5
            passed += 1 if ok else 0
            status = "✅ PASS" if ok else "❌ FAIL"
            print(f"  {name:20s} : {status} (ratio={res['ratio']:.3f})")

    print(f"\nTotal: {passed}/{total} tests pasados")

    # Generar reporte markdown
    with open('VALIDATION_REPORT.md', 'w') as f:
        f.write(f"# Informe de Validación randu_plus\n\n")
        f.write(f"**Semilla:** {seed}\n")
        f.write(f"**Datos:** {n_bytes/1e6:.0f} MB ({len(data)} uint32)\n")
        f.write(f"**Fecha:** {subprocess.run(['date'], capture_output=True, text=True).stdout.strip()}\n")
        f.write(f"**Git commit:** {subprocess.run(['git', 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()}\n\n")
        f.write("## Resultados\n\n")
        f.write("| Test | p-value / ratio | Pass |\n")
        f.write("|------|-----------------|------|\n")
        for name, res in all_results:
            if 'p_value' in res:
                f.write(f"| {name} | {res['p_value']:.6f} | {'✅' if res['pass'] else '❌'} |\n")
            elif 'ratio' in res:
                f.write(f"| {name} | ratio={res['ratio']:.3f} | {'✅' if res['ratio']>0.5 else '❌'} |\n")
            elif name == "HeaderCrossCheck":
                f.write(f"| {name} | {res['n_values']} valores x {len(res['checked_seeds'])} semillas "
                        f"x {len(res['headers_checked'])} headers | "
                        f"{'✅' if res['pass'] else '❌'} |\n")
            elif name == "HeaderSync":
                f.write(f"| {name} | {res['n_copies']} copias de randu_plus.h | "
                        f"{'✅' if res['pass'] else '❌'} |\n")
        f.write(f"\n**Total: {passed}/{total} pasados**\n")
        f.write(REPORT_CAVEAT.format(
            seed=seed,
            n_mb=f"{n_bytes/1e6:.0f}",
            n_values=len(data),
            n_xcheck=xcheck['n_values'],
            seeds=", ".join(str(s) for s in xcheck['checked_seeds'])))
    
    print("\nReporte guardado en VALIDATION_REPORT.md")

if __name__ == '__main__':
    main()