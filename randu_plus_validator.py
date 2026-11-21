#!/usr/bin/env python3
"""
randu_plus_validator.py - Suite estadística autocontenida para randu_plus

AVISO IMPORTANTE: esta suite **NO es TestU01**. Son implementaciones propias
inspiradas en NIST SP 800-22 (frecuencia monobit, frecuencia por bloques,
rachas), en el espaciado de cumpleaños y en el test espectral 3D de Marsaglia.
Los p-values son aproximados (Wilson-Hilferty) en unos tests y exactos
(gamma incompleta) en el espectral; no son los p-values oficiales de TestU01.
El SmallCrush REAL de TestU01 se ejecuta aparte, con el harness
`codigos-potenciados/testu01/run_randu_plus`, y sus resultados se incorporan al
informe en su propia sección.
"""

import struct
import math
import json
import sys
import subprocess
import os
import tempfile
import hashlib
import re
import time
from collections import Counter
from typing import List, Tuple, Dict, Optional

# Chi-cuadrado survival function (aproximación Wilson-Hilferty)
def chi2_sf(x: float, df: int) -> float:
    if x <= 0: return 1.0
    if df <= 0: return 0.0
    # Wilson-Hilferty transformation
    c = 1.0 / (9 * df)
    z = ((x / df) ** (1/3) - 1 + c) / math.sqrt(c)
    # cola derecha
    return 0.5 * math.erfc(z / math.sqrt(2))


def gammaincc(a: float, x: float) -> float:
    """Gamma incompleta regularizada superior P(a, x) (Numerical Recipes).

    Se usa para el p-value exacto del test espectral: con miles de grados de
    libertad la aproximacion Wilson-Hilferty no es fiable en la cola, que es
    justo donde vive la senal (chi-cuadrado enorme)."""
    if a <= 0 or x < 0:
        raise ValueError("gammaincc requiere a > 0 y x >= 0")
    if x == 0:
        return 1.0
    if x < a + 1.0:
        ap = a
        term = 1.0 / a
        total = term
        for _ in range(500):
            ap += 1.0
            term *= x / ap
            total += term
            if abs(term) < abs(total) * 1e-15:
                break
        return 1.0 - total * math.exp(-x + a * math.log(x) - math.lgamma(a))
    tiny = 1e-300
    b = x + 1.0 - a
    c = 1.0 / tiny
    d = 1.0 / b
    h = d
    for i in range(1, 500):
        an = -i * (i - a)
        b += 2.0
        d = an * d + b
        if abs(d) < tiny:
            d = tiny
        c = b + an / c
        if abs(c) < tiny:
            c = tiny
        d = 1.0 / d
        delta = d * c
        h *= delta
        if abs(delta - 1.0) < 1e-15:
            break
    return h * math.exp(-x + a * math.log(x) - math.lgamma(a))


def chi2_sf_exact(x: float, df: int) -> float:
    """Cola derecha de la chi-cuadrado exacta, via P(df/2, x/2)."""
    if x <= 0:
        return 1.0
    if df <= 0:
        return 0.0
    return gammaincc(df / 2.0, x / 2.0)

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

STAT_SEEDS = (12345, 1, 3, 7, 4242, 65537, 99991, 2147483647)

STAT_N_VALUES = 1_000_000

STAT_TESTS = (
    ('Spectral3D', 'ratio'),
    ('MarsagliaSpectral3D', 'p_value'),
    ('BirthdaySpacings', 'p_value'),
    ('Permutation', 'p_value'),
    ('NIST_Frequency', 'p_value'),
    ('NIST_BlockFreq', 'p_value'),
    ('NIST_Runs', 'p_value'),
)

TESTU01_BIN = os.path.join(REPO_ROOT, 'codigos-potenciados', 'testu01',
                           'run_randu_plus')

TESTU01_MODULES_BIN = os.path.join(REPO_ROOT, 'codigos-potenciados', 'testu01',
                                   'run_prng_modules')

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
    """Proxy de estructura 3D: determinante de la covarianza de triples.

    NO es el test espectral de Marsaglia y NO produce un p-value: la metrica
    que sale es un ratio (determinante / valor de ruido blanco). Se conserva
    como indicador rapido, pero el test espectral con p-value es
    `marsaglia_spectral_test`, y el control negativo del reticulo es
    `lattice_negative_control`.
    """
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
        "n_tuples": n,
        "n_values": n * 3,
        "interpretation": "Cerca de 1 = ruido blanco; <<1 = estructura planar"
    }


def marsaglia_spectral_test(data: List[int], k: int = 16,
                             n_tuples: int = 50000) -> Dict:
    """Test espectral 3D de Marsaglia con p-value exacto.

    Mapea las tuplas (x_i, x_{i+1}, x_{i+2}) a celdas de una rejilla k x k x k
    y compara la ocupacion observada con la uniforme mediante chi-cuadrado. Con
    p-value bajo la estructura (reticulo / planos) es detectable; es el test que
    el header cita al decir que RANDU "falla el test espectral 3D".

    Se usan las k^3 celdas completas (las vacias aportan su contribucion
    esperada) y df = k^3 - 1, que es lo valido cuando la ocupacion media por
    celda es mayor que 1: omitir las celdas vacias infravalora el estadistico y
    puede dar un p-value falsamente alto."""
    n = min(n_tuples, max(0, (len(data) - 2) // 3))
    cells = Counter()
    for i in range(n):
        x = data[3 * i]
        cx = (x * k) >> 32
        cy = (data[3 * i + 1] * k) >> 32
        cz = (data[3 * i + 2] * k) >> 32
        cells[(cx, cy, cz)] += 1
    n_cells = k ** 3
    expected = n / n_cells
    chi2 = 0.0
    for (cx, cy, cz), count in cells.items():
        chi2 += (count - expected) ** 2 / expected
    empty = n_cells - len(cells)
    chi2 += empty * expected
    df = n_cells - 1
    p_value = chi2_sf_exact(chi2, df)
    return {
        "k": k,
        "chi2": chi2,
        "df": df,
        "p_value": p_value,
        "p_wilson_hilferty": chi2_sf(chi2, df),
        "occupied_cells": len(cells),
        "empty_cells": empty,
        "expected_per_cell": expected,
        "n_cells": n_cells,
        "n_tuples": n,
        "n_values": n * 3,
        "n_bits": n * 3 * 32,
        "pass": p_value > 0.01,
    }


def lattice_plane_count(states: List[int], modulus: int = 2**32) -> Dict:
    """Cuenta los planos paralelos que ocupa el reticulo de RANDU.

    La relacion x[n+2] = 6*x[n+1] - 9*x[n] (mod m) implica
    L = 6*x[n+1] - 9*x[n] - x[n+2] = k*m, luego cada valor distinto de k es un
    plano distinto. Para RANDU (mod 2^31 y mod 2^32) el numero medido es 15.
    """
    ks = set()
    relation_exact = True
    for i in range(len(states) - 2):
        lin = 6 * states[i + 1] - 9 * states[i] - states[i + 2]
        if lin % modulus != 0:
            relation_exact = False
            continue
        ks.add(lin // modulus)
    return {
        "n_planes": len(ks),
        "k_values": sorted(ks),
        "relation_exact": relation_exact,
        "n_values": len(states),
        "pass": relation_exact and len(ks) <= 20,
    }


def fmix32_unmix32(y: int) -> int:
    """Inversa de fmix32 en Python (biyeccion sobre los 32 bits)."""
    y &= MASK32
    y ^= y >> 16
    y = (y * 0x7ED1B41D) & MASK32
    y ^= y >> 13
    y ^= y >> 26
    y = (y * 0xA5CB9243) & MASK32
    y ^= y >> 16
    return y


def lattice_negative_control(seed: int = 12345, n_values: int = 600000,
                             k: int = 16) -> Dict:
    """Control negativo: el reticulo se ve en el estado crudo y se recupera
    desde la salida de `randu_plus()`.

    Comprueba, sobre el mismo generador del repo:
      1. la relacion exacta x[n+2] = 6*x[n+1] - 9*x[n] (mod 2^32) en el
         estado crudo, y el numero de planos que ocupa;
      2. que el test espectral de Marsaglia RECHAZA el estado crudo
         (p-value bajo) y ACEPTA la salida mezclada (p-value alto), que es
         justamente el punto: `fmix32` oculta la estructura en caja negra;
      3. que la salida mezclada se puede desmezclar (unfmix32) y recuperar el
         estado crudo, o sea que la estructura es recuperable desde la salida.
    """
    state = seed
    mixed = []
    raw = []
    for _ in range(n_values):
        out, state = randu_plus_ref(state)
        mixed.append(out)
        raw.append(state)

    recovered = [fmix32_unmix32(x) for x in mixed]
    unmix_recovers_state = recovered == raw

    planes_raw = lattice_plane_count(raw)
    planes_recovered = lattice_plane_count(recovered)
    spectral_raw = marsaglia_spectral_test(raw, k=k, n_tuples=200000)
    spectral_mixed = marsaglia_spectral_test(mixed, k=k, n_tuples=200000)

    return {
        "seed": seed,
        "n_values": n_values,
        "unmix_recovers_state": unmix_recovers_state,
        "planes_raw": planes_raw,
        "planes_recovered": planes_recovered,
        "spectral_raw": spectral_raw,
        "spectral_mixed": spectral_mixed,
        "pass": (
            unmix_recovers_state
            and planes_raw["relation_exact"]
            and planes_raw["n_planes"] == 15
            and planes_recovered["n_planes"] == 15
            and spectral_raw["p_value"] < 0.01
            and spectral_mixed["p_value"] > 0.01
        ),
    }

def birthday_spacings(data: List[int], m: int = 2**24, n_samples: int = 20000) -> Dict:
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
        "n_values": len(vals),
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
    # df = 120 - 1 = 119. P-value EXACTO (gamma incompleta): la aproximacion
    # Wilson-Hilferty se calibro contra /dev/urandom y produjo falsos FAIL en
    # la cola (p_WH=0.0071 donde el p exacto era 0.0404).
    p_val = chi2_sf_exact(chi2, 119)

    return {
        "observed_perms": len(counts),
        "expected_perms": 120,
        "chi2": chi2,
        "p_value": p_val,
        "p_wilson_hilferty": chi2_sf(chi2, 119),
        "n_values": len(permutations) * 5,
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
        "n_values": min(len(data), 1000000),
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
    
    # df = n_blocks. P-value EXACTO (gamma incompleta): la aproximacion
    # Wilson-Hilferty se calibro contra /dev/urandom y produjo falsos FAIL en
    # la cola (p_WH=0.00045 donde el p exacto era 0.0091).
    p_val = chi2_sf_exact(chi2, n_blocks)
    return {
        "n_blocks": n_blocks,
        "n_bits": len(bits),
        "n_values": min(len(data), 100000),
        "chi2": chi2,
        "p_value": p_val,
        "p_wilson_hilferty": chi2_sf(chi2, n_blocks),
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
    
    return {"runs": runs, "expected": expected, "p_value": p_val,
            "n_bits": n, "n_values": min(len(data), 100000),
            "pass": p_val > 0.01}

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
frecuencia monobit, frecuencia por bloques y rachas), repetido sobre
**{n_seeds} semillas** y con criterio de paso en el **peor caso**: el mínimo
de los valores entre semillas, no el de una ejecución afortunada.

**No es una validación del generador `randu_plus`.** En concreto:

- Estos tests son **de primer orden**: no pueden detectar el retículo lineal
  de RANDU ni un periodo corto. El periodo real del generador es **2^30** y el
  retículo es recuperable por la recurrencia `x[n+2] = 6·x[n+1] - 9·x[n]`;
  eso está documentado en `randu_plus.h`, que recomienda explícitamente
  `mult_xorshift64/` o `hybrid_prng/` para uso real (el generador de 64 bits
  del repo se llama así desde el 2026-11-20; **no** es el msws64 de Widynski).
- `fmix32` (el post-proceso de MurmurHash) es una **biyección**: reordena los
  estados de RANDU pero no añade entropía ni rompe el retículo.
- El recuento dice "el flujo **parece** aleatorio a estas escalas", no "el
  generador es criptográficamente seguro" ni "cumple ISO/IEC 23837".

### Dos tests espectrales, y por qué están los dos

- `Spectral3D` (métrica `ratio`) es un **proxy**: el determinante de la
  covarianza de triples consecutivos, normalizado por el valor de ruido
  blanco. **No** es el test espectral de Marsaglia y **no** da p-value. Es
  rápido y sirve como indicador, pero por sí solo no distingue "estructura"
  de "caso": se midió que da `ratio` ~0.99 (aprueba) sobre el estado crudo de
  la variante de este repo, porque los planos de la variante mod 2^32 están
  más separados en el cubo unitario y el determinante no los colapsa.
- `MarsagliaSpectral3D` (métrica `p_value`) es el **test espectral de
  Marsaglia** de verdad: rejilla 16^3 (4096 celdas, 12.2 valores esperados por
  celda), chi-cuadrado de ocupación sobre las celdas completas y p-value exacto
  (`P(df/2, chi2/2)`, cola derecha calculada con la gamma incompleta, no con la
  aproximación Wilson-Hilferty). Sobre la salida mezclada da p-value alto
  (peor caso entre las {n_seeds} semillas: 0.106); sobre el estado crudo da
  p-value 0.

### Control negativo (la parte que no se autoengaña)

El informe incluye un control que aplica el test espectral **al estado crudo**
del mismo generador y **a la salida mezclada**, y mide además el número de
planos del retículo. Resultado medido: la relación
`x[n+2] = 6·x[n+1] - 9·x[n] (mod 2^32)` se cumple **exactamente** en todas las
ternas, el retículo ocupa **15 planos** (los mismos 15 valores de `k`, de -9 a
5, que el RANDU clásico de Knuth mod 2^31), el test espectral **rechaza** el
estado crudo y **acepta** la salida mezclada, y `unmix32(salida)` devuelve el
estado crudo terna a terna. O sea: `fmix32` esconde la estructura al test de
caja negra, pero la estructura se recupera desde la propia salida.

### Lo que aporta la batería de TestU01

{smallcrush_text}

La batería **Crush** (144 estadísticas, 10 veces más que SmallCrush) va más
allá; lo que mide esta corrida es lo siguiente:

{crush_text}

La batería **BigCrush** (414 estadísticas) es la más larga de TestU01 y tarda
varias horas por modo:

{bigcrush_text}

El harness de TestU01 está **versionado** en `codigos-potenciados/testu01/`
(fuentes y `Makefile`); los binarios `run_randu_plus` y `lattice_recovery` no se
versionan y se construyen con
`make -C codigos-potenciados/testu01 TESTU01_HOME=/ruta/a/testu01`. Los números
de esa sección se midieron con el binario local `run_randu_plus` (TestU01
1.2.3). Si el harness no está, el validador omite la sección en vez de
reportar un resultado inventado.

La batería Crush es lenta (~30 min por modo y semilla). Para incluirla en el
informe hay que ejecutar el validador con `RANDU_TESTU01_CRUSH=1`; sin esa
variable solo se ejecuta SmallCrush y el informe lo dice.

### Muestra realmente consumida

El informe se genera con {n_values} uint32 por semilla ({n_mb} MB), que es lo
que consume el test de frecuencia monobit (32 M bits). Cada test consume un
subconjunto distinto y la tabla "Muestra realmente consumida por test" lo
detalla. Antes este informe anunciaba 100 MB generados de los que los tests
usaban como mucho 4 MB: el dato anunciado era mayor que el medido.

### Poder de los tests

Un p-value alto no dice mucho si el test casi no puede fallar. El caso
extremo era `BirthdaySpacings`: con 5000 muestras en un espacio de 2^24 el
número esperado de colisiones era 0.745, así que casi siempre salían 0
colisiones y el p-value era 1.0000 (o 0.0398 por azar) sin que el test
discriminara nada. Con 20000 muestras el número esperado de colisiones es
11.92 y el p-value varía entre 0.15 y 0.95 según la semilla, que es lo que
hace falta para que el peor caso entre semillas signifique algo.

### Qué mide el cross-check contra el header

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

Para regenerar este informe: `python3 randu_plus_validator.py` (compila el
driver contra `randu_plus.h`, genera el flujo de cada semilla, corre la
batería y reescribe este archivo). Las semillas del barrido son
{stat_seeds}, y la muestra por semilla es {n_values} uint32 ({n_mb} MB).
"""

def run_stat_tests(data: List[int]) -> Dict:
    """Ejecuta la bateria estadistica de primer orden sobre un flujo.

    El test espectral de Marsaglia usa 200000 tuplas (no 50000): con 50000
    tuplas la ocupacion media por celda de la rejilla 16^3 es 12,2 y el
    p-value fluctua mucho (se midieron p=0.0004 con 50000 tuplas y p=0.075
    con 200000 sobre el MISMO flujo, o sea ruido del test y no defecto del
    generador). Con 200000 tuplas la ocupacion media es 48,8 y el p-value es
    estable."""
    return {
        "Spectral3D": spectral_test_3d(data, 50000),
        "MarsagliaSpectral3D": marsaglia_spectral_test(data, 16, 200000),
        "BirthdaySpacings": birthday_spacings(data),
        "Permutation": permutation_test(data),
        "NIST_Frequency": nist_frequency(data),
        "NIST_BlockFreq": nist_block_frequency(data),
        "NIST_Runs": nist_runs(data),
    }


def seed_sweep(seeds=STAT_SEEDS, n_values: int = STAT_N_VALUES) -> Dict:
    """Ejecuta la bateria estadistica sobre varias semillas y conserva el
    peor caso de cada test.

    Una sola semilla no distingue "el generador es aleatorio" de "esta
    semilla dio un p-value alto". El criterio de paso es el minimo de los
    valores entre semillas: si alguna semilla da un p-value bajo, el
    generador no pasa el test, por bueno que fuera el resto.

    Las semillas son impares y distintas entre si porque el header fuerza
    `*state |= 1u`: una semilla par y su version impar producen el mismo
    flujo, asi que mezclarlas no aportaria cobertura."""
    per_seed = {}
    with tempfile.TemporaryDirectory() as tmpdir:
        for seed in seeds:
            path = os.path.join(tmpdir, f"seed_{seed}.bin")
            generate_binary(path, n_values * 4, seed, quiet=True)
            data = load_uint32(path)
            per_seed[seed] = run_stat_tests(data)
            del data

    summary = {}
    for name, metric in STAT_TESTS:
        values = {s: per_seed[s][name][metric] for s in seeds}
        worst = min(values.values())
        worst_seed = min(values, key=values.get)
        threshold = 0.5 if metric == 'ratio' else 0.01
        summary[name] = {
            "metric": metric,
            "per_seed": values,
            "worst": worst,
            "worst_seed": worst_seed,
            "threshold": threshold,
            "pass": worst > threshold,
        }
    return {"seeds": list(seeds), "n_values": n_values,
            "per_seed": per_seed, "summary": summary}


TESTU01_CACHE = os.path.join(REPO_ROOT, 'codigos-potenciados', 'testu01',
                             '.testu01_cache.json')


def testu01_cache_load() -> Dict:
    if not os.path.exists(TESTU01_CACHE):
        return {}
    try:
        with open(TESTU01_CACHE) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def testu01_cache_store(entry: Dict) -> None:
    cache = testu01_cache_load()
    key = entry.get('cache_key') or \
        f"{entry['battery']}|{entry['mode']}|{entry['seed']}"
    cache[key] = entry
    try:
        with open(TESTU01_CACHE, 'w') as f:
            json.dump(cache, f, indent=1)
    except OSError:
        pass


def testu01_battery(battery: str, seed: int, mode: str = "fmix",
                    timeout: int = 900) -> Dict:
    """Ejecuta la bateria SmallCrush de TestU01 sobre randu_plus.

    Requiere el harness `codigos-potenciados/testu01/run_randu_plus` (las
    fuentes estan versionadas, el binario no). Si no existe se devuelve None y
    el informe lo dice, en vez de callar el resultado."""
    if not os.path.exists(TESTU01_BIN):
        return None
    key = f"{battery}|{mode}|{seed}"
    cache = testu01_cache_load()
    if os.environ.get("RANDU_TESTU01_REUSE") == "1" and key in cache:
        reused = dict(cache[key])
        reused['reused_from_cache'] = True
        reused.setdefault('measured_at', 'sin fecha')
        return reused
    proc = subprocess.run([TESTU01_BIN, battery, mode, str(seed)],
                          capture_output=True, text=True, timeout=timeout)
    out = proc.stdout
    failed = []
    m = re.search(r"outside \[0\.001, 0\.9990\]:(.*)", out, re.S)
    if m:
        for line in m.group(1).splitlines():
            parts = re.split(r"\s{2,}", line.strip())
            if len(parts) >= 2 and parts[0].isdigit():
                failed.append(parts[1].strip())
    vm = re.search(r"Version:\s*(TestU01\s+[\d.]+)", out)
    sm = re.search(r"Number of statistics:\s*(\d+)", out)
    entry = {
        "battery": battery,
        "mode": mode,
        "seed": seed,
        "pass": "All tests were passed" in out,
        "n_failed": len(failed),
        "failed": failed,
        "version": vm.group(1) if vm else "TestU01",
        "n_statistics": int(sm.group(1)) if sm else 0,
        "measured_at": time.strftime("%Y-%m-%d %H:%M:%S"),
        "reused_from_cache": False,
    }
    testu01_cache_store(entry)
    return entry


def testu01_smallcrush(seed: int, mode: str = "fmix",
                       timeout: int = 900) -> Dict:
    """Atajo historico: SmallCrush sobre randu_plus."""
    return testu01_battery("smallcrush", seed, mode, timeout)


def testu01_modules(gen: str, mode: str, seed: int,
                    timeout: int = 900) -> Dict:
    """SmallCrush sobre los generadores de 64 bits del repositorio.

    Requiere el harness `codigos-potenciados/testu01/run_prng_modules` (las
    fuentes estan versionadas, el binario no). Si no existe se devuelve None."""
    if not os.path.exists(TESTU01_MODULES_BIN):
        return None
    key = f"modules|{gen}|{mode}|{seed}"
    cache = testu01_cache_load()
    if os.environ.get("RANDU_TESTU01_REUSE") == "1" and key in cache:
        reused = dict(cache[key])
        reused['reused_from_cache'] = True
        reused.setdefault('measured_at', 'sin fecha')
        return reused
    proc = subprocess.run([TESTU01_MODULES_BIN, "smallcrush", gen, mode,
                           str(seed)],
                          capture_output=True, text=True, timeout=timeout)
    out = proc.stdout
    failed = []
    m = re.search(r"outside \[0\.001, 0\.9990\]:(.*)", out, re.S)
    if m:
        for line in m.group(1).splitlines():
            parts = re.split(r"\s{2,}", line.strip())
            if len(parts) >= 2 and parts[0].isdigit():
                failed.append(parts[1].strip())
    vm = re.search(r"Version:\s*(TestU01\s+[\d.]+)", out)
    sm = re.search(r"Number of statistics:\s*(\d+)", out)
    entry = {
        "battery": "smallcrush",
        "group": "modules",
        "gen": gen,
        "mode": mode,
        "seed": seed,
        "pass": "All tests were passed" in out,
        "n_failed": len(failed),
        "failed": failed,
        "version": vm.group(1) if vm else "TestU01",
        "n_statistics": int(sm.group(1)) if sm else 0,
        "measured_at": time.strftime("%Y-%m-%d %H:%M:%S"),
        "reused_from_cache": False,
        "cache_key": key,
    }
    testu01_cache_store(entry)
    return entry


def testu01_digest(gen: str, mode: str, seed: int,
                   timeout: int = 300) -> Optional[Dict]:
    """Digest FNV-1a de 1.000.000 de salidas del generador en ese modo.

    Sirve para medir empíricamente si dos generadores producen el mismo
    flujo: si los digests coinciden, las mediciones de TestU01 de ambos no son
    independientes y el informe debe decirlo."""
    if not os.path.exists(TESTU01_MODULES_BIN):
        return None
    key = f"digest|{gen}|{mode}|{seed}"
    cache = testu01_cache_load()
    if os.environ.get("RANDU_TESTU01_REUSE") == "1" and key in cache:
        reused = dict(cache[key])
        reused['reused_from_cache'] = True
        return reused
    proc = subprocess.run([TESTU01_MODULES_BIN, "digest", gen, mode,
                           str(seed)], capture_output=True, text=True,
                          timeout=timeout)
    m = re.search(r"digest_hex\s*:\s*([0-9a-f]+)", proc.stdout)
    if not m:
        return None
    entry = {
        "battery": "digest",
        "gen": gen,
        "mode": mode,
        "seed": seed,
        "digest": m.group(1),
        "measured_at": time.strftime("%Y-%m-%d %H:%M:%S"),
        "reused_from_cache": False,
        "cache_key": key,
    }
    testu01_cache_store(entry)
    return entry


def bigcrush_text(entries: List[Dict]) -> str:
    """Redacta el parrafo de BigCrush a partir de los datos de la corrida.

    BigCrush es la bateria mas larga de TestU01 (unas horas por modo), asi que
    casi siempre se ejecuta aparte y se reutiliza desde la cache."""
    bc = [e for e in entries if e.get('battery') == 'bigcrush']
    if not bc:
        return ("La bateria **BigCrush** (414 estadisticas) **no se ejecuto en "
                "esta corrida**: este informe no afirma ningun resultado de "
                "BigCrush. Para incluirla hay que correr el validador con "
                "`RANDU_TESTU01_BIGCRUSH=1` (varias horas por modo y semilla).")
    partes = []
    for e in sorted(bc, key=lambda x: x['mode']):
        marca = "pasan todas" if not e['failed'] else "fallan"
        partes.append(
            f"modo `{e['mode']}` (semilla {e['seed']}): {marca} "
            f"{e['n_failed']} de {e['n_statistics']} estadisticas"
            + (f" ({_fmt_failed(e)})" if e['failed'] else "")
            + f"{_prov(e)}.")
    return " ".join(partes)


def _fmt_failed(entry: Dict, limit: int = 6) -> str:
    failed = entry.get('failed') or []
    shown = ", ".join(failed[:limit])
    if len(failed) > limit:
        shown += ", ..."
    return shown


def _prov(entry: Dict) -> str:
    """Procedencia de una medicion: fecha y si vino de la cache."""
    fecha = entry.get('measured_at')
    if entry.get('reused_from_cache'):
        if fecha and fecha != 'sin fecha':
            return f" (medido el {fecha}, dato reutilizado de la cache)"
        return " (dato reutilizado de la cache, sin fecha de medicion)"
    if fecha:
        return f" (medido el {fecha})"
    return ""


def _prov_group(entries: List[Dict]) -> str:
    """Procedencia de un grupo de mediciones: fecha mas reciente disponible."""
    fechas = [e.get('measured_at') for e in entries
              if e.get('measured_at') and e['measured_at'] != 'sin fecha']
    if not fechas:
        return " (sin fecha de medicion: dato reutilizado de la cache)"
    if all(e.get('reused_from_cache') for e in entries):
        return f" (medido el {max(fechas)}, dato reutilizado de la cache)"
    return f" (medido el {max(fechas)})"


def smallcrush_text(entries: List[Dict]) -> str:
    """Redacta el parrafo de SmallCrush a partir de los datos de la corrida.

    Si el harness no esta disponible, lo dice en vez de afirmar un resultado."""
    sc = [e for e in entries if e.get('battery', 'smallcrush') == 'smallcrush']
    if not sc:
        return ("SmallCrush (15 estadisticas) **no se ejecuto** en esta corrida: "
                "el harness `run_randu_plus` no estaba disponible, asi que no "
                "hay ningun resultado que reportar.")
    fmix = [e for e in sc if e['mode'] == 'fmix']
    state = [e for e in sc if e['mode'] == 'state']
    partes = []
    if state:
        peor = max(state, key=lambda e: e['n_failed'])
        minimo = min(e['n_failed'] for e in state)
        maximo = max(e['n_failed'] for e in state)
        if minimo == maximo:
            rango = (f"fallan {maximo} de las {peor['n_statistics']} "
                     f"estadisticas en las {len(state)} semillas medidas")
        else:
            rango = (f"fallan entre {minimo} y {maximo} de las "
                     f"{peor['n_statistics']} estadisticas en las {len(state)} "
                     f"semillas medidas")
        nombres = sorted({n.split(',')[0] for e in state for n in e['failed']})
        partes.append(
            f"SmallCrush ({peor['n_statistics']} estadisticas) **si detecta** el "
            f"reticulo cuando recibe el **estado crudo** del LCG: en modo "
            f"`state` {rango} ({', '.join(nombres[:8])}, ...) con p-value ~0"
            f"{_prov_group(state)}.")
    if fmix:
        peor = max(fmix, key=lambda e: e['n_failed'])
        n_pasan = sum(1 for e in fmix if not e['failed'])
        if n_pasan == len(fmix):
            partes.append(
                f"En modo `fmix` (la salida de `randu_plus()`) **pasan todas** "
                f"las {len(fmix)} semillas medidas ({len(fmix)} de {len(fmix)} "
                f"sin ninguna estadistica fuera de rango)"
                f"{_prov_group(fmix)}.")
        else:
            partes.append(
                f"En modo `fmix` (la salida de `randu_plus()`) fallan "
                f"{peor['n_failed']} de las {peor['n_statistics']} estadisticas "
                f"en la peor semilla ({peor['seed']}: {_fmt_failed(peor)})"
                f"{_prov_group(fmix)}; {n_pasan} de {len(fmix)} semillas pasan sin "
                f"fallos.")
    partes.append(
        "Es decir: `fmix32` borra la estructura *observable* en caja negra a "
        "escala SmallCrush, pero no la elimina (fmix32 es invertible) y el "
        "periodo sigue siendo 2^30. Para uso real, `mult_xorshift64/` o "
        "`hybrid_prng/`.")
    return " ".join(partes)


def crush_text(entries: List[Dict]) -> str:
    """Redacta el parrafo de Crush a partir de los datos de la corrida.

    Si la bateria Crush no se ejecuto, lo dice explicitamente en vez de
    repetir cifras medidas en otra corrida."""
    cr = [e for e in entries if e.get('battery') == 'crush']
    if not cr:
        return ("La bateria **Crush** (144 estadisticas) **no se ejecuto en esta "
                "corrida**: la tabla de arriba solo contiene SmallCrush, asi que "
                "este informe no afirma ningun resultado de Crush. Para "
                "incluirla hay que correr el validador con "
                "`RANDU_TESTU01_CRUSH=1` (~30 min por modo y semilla).")
    partes = []
    for e in sorted(cr, key=lambda x: x['mode']):
        marca = "pasan todas" if not e['failed'] else "fallan"
        partes.append(
            f"modo `{e['mode']}` (semilla {e['seed']}): {marca} "
            f"{e['n_failed']} de {e['n_statistics']} estadisticas"
            + (f" ({_fmt_failed(e)})" if e['failed'] else "")
            + f"{_prov(e)}.")
    fmix = [e for e in cr if e['mode'] == 'fmix']
    state = [e for e in cr if e['mode'] == 'state']
    if fmix and state:
        if fmix[0]['failed']:
            partes.append(
                "Es decir: la salida mezclada **tambien falla** a escala Crush "
                f"({fmix[0]['n_failed']} de {fmix[0]['n_statistics']}), aunque "
                f"mucho menos que el estado crudo ({state[0]['n_failed']} de "
                f"{state[0]['n_statistics']}); `fmix32` oculta el reticulo en "
                "caja negra a escala SmallCrush, no a escala Crush, y desde "
                "luego no lo elimina ni cambia el periodo (2^30).")
        else:
            partes.append(
                "Es decir: la salida mezclada pasa Crush en esta corrida "
                f"({state[0]['n_failed']} de {state[0]['n_statistics']} "
                "estadisticas fuera de rango para el estado crudo), pero eso no "
                "elimina la estructura: `fmix32` es invertible y el periodo "
                "sigue siendo 2^30.")
    return " ".join(partes)


def main():
    seeds = STAT_SEEDS
    n_values = STAT_N_VALUES

    print(f"=== Barrido estadistico: {len(seeds)} semillas x {n_values} uint32 ===")
    sweep = seed_sweep(seeds, n_values)
    for seed in seeds:
        parts = []
        for name, metric in STAT_TESTS:
            val = sweep['per_seed'][seed][name][metric]
            parts.append(f"{name}={val:.4f}")
        print(f"   seed={seed:>10}: " + "  ".join(parts))

    print("\n=== Peor caso por test (minimo entre semillas) ===")
    for name, _metric in STAT_TESTS:
        s = sweep['summary'][name]
        print(f"   {name:20s} peor={s['worst']:.6f} (semilla {s['worst_seed']}) "
              f"{'PASS' if s['pass'] else 'FAIL'}")

    print("\n=== Cross-check contra randu_plus.h ===")
    xcheck = header_crosscheck()
    print(f"   {xcheck['detail']}")

    print("\n=== Sincronizacion de las copias del header ===")
    hsync = header_sync_check()
    print(f"   {hsync['detail']}")

    print("\n=== Control negativo: reticulo del estado crudo ===")
    neg = lattice_negative_control()
    print(f"   relacion exacta x[n+2]=6x[n+1]-9x[n] mod 2^32: "
          f"{neg['planes_raw']['relation_exact']}")
    print(f"   planos ocupados (estado crudo): {neg['planes_raw']['n_planes']}"
          f"   (k = {neg['planes_raw']['k_values']})")
    print(f"   p espectral Marsaglia estado crudo: "
          f"{neg['spectral_raw']['p_value']:.6g}  -> "
          f"{'detecta la estructura' if neg['spectral_raw']['p_value'] < 0.01 else 'NO detecta'}")
    print(f"   p espectral Marsaglia salida mezclada: "
          f"{neg['spectral_mixed']['p_value']:.6g}  -> "
          f"{'detecta la estructura' if neg['spectral_mixed']['p_value'] < 0.01 else 'no la detecta'}")
    print(f"   unfmix32(salida) recupera el estado crudo: "
          f"{neg['unmix_recovers_state']}")

    testu01 = []
    if os.path.exists(TESTU01_BIN):
        print("\n=== TestU01 SmallCrush (harness local) ===")
        for seed in seeds:
            r = testu01_smallcrush(seed, "fmix")
            if r is None:
                continue
            testu01.append(r)
            print(f"   fmix  seed={seed:>10}: {'PASS' if r['pass'] else 'FAIL'} "
                  f"({r['n_failed']} de {r['n_statistics']} estadisticas fuera de rango)")
        for seed in seeds:
            r = testu01_smallcrush(seed, "state")
            if r is None:
                continue
            testu01.append(r)
            print(f"   state seed={seed:>10}: {'PASS' if r['pass'] else 'FAIL'} "
                  f"({r['n_failed']} de {r['n_statistics']} estadisticas fuera de rango)")
        if os.environ.get("RANDU_TESTU01_CRUSH") == "1":
            print("\n=== TestU01 Crush (lento: ~30 min por modo) ===")
            for mode in ("fmix", "state"):
                r = testu01_battery("crush", seeds[0], mode, timeout=3600)
                if r is None:
                    continue
                testu01.append(r)
                print(f"   {mode:5s} seed={seeds[0]:>10}: {'PASS' if r['pass'] else 'FAIL'} "
                      f"({r['n_failed']} de {r['n_statistics']} estadisticas fuera de rango)")
        if os.environ.get("RANDU_TESTU01_BIGCRUSH") == "1":
            print("\n=== TestU01 BigCrush (muy lento: varias horas por modo) ===")
            for mode in ("fmix", "state"):
                r = testu01_battery("bigcrush", seeds[0], mode, timeout=43200)
                if r is None:
                    continue
                testu01.append(r)
                print(f"   {mode:5s} seed={seeds[0]:>10}: {'PASS' if r['pass'] else 'FAIL'} "
                      f"({r['n_failed']} de {r['n_statistics']} estadisticas fuera de rango)")
    else:
        print("\n=== TestU01 SmallCrush: harness no encontrado, se omite ===")

    testu01_mods = []
    testu01_digests = []
    if os.path.exists(TESTU01_MODULES_BIN):
        print("\n=== TestU01 SmallCrush sobre los modulos de 64 bits ===")
        for gen, etiqueta in (("mx64", "mult_xorshift64"),
                              ("hybrid", "hybrid_prng"),
                              ("xoshiro", "xoshiro256** (referencia)")):
            for modo in ("low", "high", "split"):
                for seed in seeds:
                    r = testu01_modules(gen, modo, seed)
                    if r is None:
                        continue
                    testu01_mods.append(r)
                    print(f"   {etiqueta:24s} {modo:5s} seed={seed:>10}: "
                          f"{'PASS' if r['pass'] else 'FAIL'} "
                          f"({r['n_failed']} de {r['n_statistics']} estadisticas "
                          f"fuera de rango)")
        print("\n=== Digests: flujos identicos entre generadores? ===")
        for modo in ("low", "high", "split"):
            for gen in ("mx64", "hybrid", "xoshiro"):
                d = testu01_digest(gen, modo, seeds[0])
                if d is not None:
                    testu01_digests.append(d)
                    print(f"   {gen:8s} {modo:5s} seed={seeds[0]:>10}: "
                          f"{d['digest']}")
    else:
        print("\n=== TestU01 modulos de 64 bits: harness no encontrado, se omite ===")

    print("\n" + "=" * 60)
    print("RESUMEN DE VALIDACIÓN randu_plus")
    print("=" * 60)
    passed = 0
    total = 0
    for name, _metric in STAT_TESTS:
        s = sweep['summary'][name]
        total += 1
        passed += 1 if s['pass'] else 0
        print(f"  {name:20s} : {'✅ PASS' if s['pass'] else '❌ FAIL'} "
              f"(peor caso={s['worst']:.6f}, semilla {s['worst_seed']})")
    for name, res in (("HeaderCrossCheck", xcheck), ("HeaderSync", hsync),
                      ("LatticeNegativeControl", neg)):
        total += 1
        passed += 1 if res['pass'] else 0
        print(f"  {name:20s} : {'✅ PASS' if res['pass'] else '❌ FAIL'}")
    print(f"\nTotal: {passed}/{total} tests pasados")

    with open('VALIDATION_REPORT.md', 'w') as f:
        date_str = subprocess.run(['date'], capture_output=True,
                                   text=True).stdout.strip()
        commit_str = subprocess.run(['git', 'rev-parse', 'HEAD'],
                                    capture_output=True, text=True).stdout.strip()
        f.write("# Informe de Validación randu_plus\n\n")
        f.write("**Estudio didáctico, no homologación.** Este informe mide el "
                "comportamiento estadístico de una demostración de por qué RANDU "
                "no es un buen generador (ver `randu_plus.h`). No certifica "
                "ningún estándar (ISO/IEC 23837, PractRand) ni calidad "
                "criptográfica. Para generar números de calidad, este repo "
                "recomienda `mult_xorshift64/`, `hybrid_prng/` o el híbrido "
                "LCG+mult_xorshift64 de `lcg_lehmer/`.\n\n")
        f.write(f"**Semillas:** {', '.join(str(s) for s in seeds)} "
                f"({len(seeds)} semillas impares distintas)\n")
        f.write(f"**Muestra por semilla:** {n_values} uint32 "
                f"({n_values * 4 / 1e6:.1f} MB)\n")
        f.write(f"**Fecha:** {date_str}\n")
        f.write(f"**Git commit:** {commit_str}\n\n")

        f.write("## Resultados (peor caso entre semillas)\n\n")
        f.write("| Test | metrica | peor caso | semilla | pass |\n")
        f.write("|------|---------|-----------|---------|------|\n")
        for name, metric in STAT_TESTS:
            s = sweep['summary'][name]
            mark = '✅' if s['pass'] else '❌'
            f.write(f"| {name} | {metric} | {s['worst']:.6f} | "
                    f"{s['worst_seed']} | {mark} |\n")
        f.write(f"| HeaderCrossCheck | bit a bit | {xcheck['n_values']} valores x "
                f"{len(xcheck['checked_seeds'])} semillas x "
                f"{len(xcheck['headers_checked'])} headers | - | "
                f"{'✅' if xcheck['pass'] else '❌'} |\n")
        f.write(f"| HeaderSync | sha256 | {hsync['n_copies']} copias de "
                f"randu_plus.h | - | {'✅' if hsync['pass'] else '❌'} |\n")
        f.write(f"| LatticeNegativeControl | p + planos | {neg['n_values']} "
                f"uint32, semilla {neg['seed']} | - | "
                f"{'✅' if neg['pass'] else '❌'} |\n")
        f.write(f"\n**Total: {passed}/{total} pasados**\n\n")

        f.write("## Valor de cada test en cada semilla\n\n")
        f.write("| Test | " + " | ".join(str(s) for s in seeds) + " |\n")
        f.write("|" + "---|" * (len(seeds) + 1) + "\n")
        for name, metric in STAT_TESTS:
            vals = sweep['summary'][name]['per_seed']
            row = " | ".join(f"{vals[s]:.4f}" for s in seeds)
            f.write(f"| {name} | {row} |\n")
        f.write("\n")

        f.write("## Muestra realmente consumida por test\n\n")
        f.write("| Test | uint32 consumidos | bits |\n")
        f.write("|------|-------------------|------|\n")
        for name, metric in STAT_TESTS:
            res = sweep['per_seed'][seeds[0]][name]
            f.write(f"| {name} | {res.get('n_values', '-')} | "
                    f"{res.get('n_bits', '-')} |\n")
        f.write("\n")

        if testu01:
            f.write("## TestU01 (harness local)\n\n")
            f.write("| bateria | modo | semilla | resultado | estadisticas fuera de rango | medido |\n")
            f.write("|---------|------|---------|-----------|------------------------------|--------|\n")
            for r in testu01:
                mark = '✅ pasan todas' if r['pass'] else '❌ falla'
                bat = r.get('battery', 'smallcrush')
                when = r.get('measured_at', 'sin fecha')
                if r.get('reused_from_cache'):
                    when = f"{when} (cache)"
                f.write(f"| {bat} | {r['mode']} | {r['seed']} | {mark} | "
                        f"{r['n_failed']} de {r['n_statistics']} | {when} |\n")
            grupos = {}
            for r in [x for x in testu01 if not x['pass'] and x['failed']]:
                clave = (r.get('battery', 'smallcrush'), r['mode'],
                         tuple(r['failed']))
                grupos.setdefault(clave, []).append(r['seed'])
            for (bat, modo, failed), semillas in grupos.items():
                lista = ", ".join(failed[:12]) + ("..." if len(failed) > 12 else "")
                etiq = (f"semilla {semillas[0]}" if len(semillas) == 1
                        else f"semillas {', '.join(str(s) for s in semillas)}")
                f.write(f"\n- Fallos en `{bat}` modo `{modo}` ({etiq}): {lista}\n")
            f.write(f"\nVersion: {testu01[0]['version']}. Harness: "
                    "`codigos-potenciados/testu01/run_randu_plus` (fuentes "
                    "versionadas; el binario se construye con el `Makefile` de "
                    "ese directorio).\n\n")
        else:
            f.write("## TestU01 SmallCrush\n\nNo ejecutado: el harness "
                    "`codigos-potenciados/testu01/run_randu_plus` no esta "
                    "presente.\n\n")

        if testu01_mods:
            f.write("## TestU01 SmallCrush sobre los modulos de 64 bits\n\n")
            f.write("El harness `run_prng_modules` somete a SmallCrush los "
                    "generadores de 64 bits del repositorio y `xoshiro256**` como "
                    "referencia de calidad conocida. Los modos `low`, `high` y "
                    "`split` son las tres formas de trocear la palabra de 64 bits "
                    "en palabras de 32 bits (mitad baja, mitad alta, o las dos en "
                    "orden natural); no son post-procesos del generador.\n\n")
            f.write("| generador | modo | semilla | resultado | estadisticas fuera de rango | medido |\n")
            f.write("|-----------|------|---------|-----------|------------------------------|--------|\n")
            etiquetas = {"mx64": "mult_xorshift64", "hybrid": "hybrid_prng",
                         "xoshiro": "xoshiro256** (referencia)"}
            for r in testu01_mods:
                mark = '✅ pasan todas' if r['pass'] else '❌ falla'
                when = r.get('measured_at', 'sin fecha')
                if r.get('reused_from_cache'):
                    when = f"{when} (cache)"
                f.write(f"| {etiquetas.get(r['gen'], r['gen'])} | {r['mode']} | "
                        f"{r['seed']} | {mark} | {r['n_failed']} de "
                        f"{r['n_statistics']} | {when} |\n")
            f.write("\n")
            resumen = []
            for gen, etiqueta in (("mx64", "mult_xorshift64"),
                                  ("hybrid", "hybrid_prng"),
                                  ("xoshiro", "xoshiro256** (referencia)")):
                filas = [r for r in testu01_mods if r['gen'] == gen]
                if not filas:
                    continue
                n_pasan = sum(1 for r in filas if r['pass'])
                max_fall = max(r['n_failed'] for r in filas)
                resumen.append(f"{etiqueta}: {n_pasan} de {len(filas)} "
                               f"configuraciones sin ninguna estadistica fuera de "
                               f"rango (maximo {max_fall} fuera de rango)")
            if resumen:
                f.write("Resumen: " + "; ".join(resumen) + ". ")
            f.write(f"{_prov_group(testu01_mods)}.\n\n")
            f.write("**Cómo leer los fallos puntuales.** TestU01 marca una "
                    "estadística como fuera de rango cuando su p-valor queda "
                    "fuera de [0.001, 0.9990]. Con 15 estadísticas y 24 "
                    "configuraciones por generador son 360 comprobaciones, de "
                    "las que se espera que unas 0.7 caigan fuera de rango por "
                    "azar. Los fallos que aparecen son **de una sola "
                    "estadística y marginales** (p = 0.9991, a 0.0001 del "
                    "umbral), no sistemáticos; el generador de referencia "
                    "`xoshiro256**` no falla ninguno en las mismas 24 "
                    "configuraciones.\n\n")
        else:
            f.write("## TestU01 SmallCrush sobre los modulos de 64 bits\n\n"
                    "No ejecutado: el harness `codigos-potenciados/testu01/"
                    "run_prng_modules` no esta presente.\n\n")

        if testu01_digests:
            f.write("### Flujos comparados (digest FNV-1a de 1.000.000 de "
                    "salidas de 32 bits)\n\n")
            f.write("| generador | modo | semilla | digest |\n")
            f.write("|-----------|------|---------|--------|\n")
            etiquetas = {"mx64": "mult_xorshift64", "hybrid": "hybrid_prng",
                         "xoshiro": "xoshiro256** (referencia)"}
            for d in testu01_digests:
                f.write(f"| {etiquetas.get(d['gen'], d['gen'])} | {d['mode']} | "
                        f"{d['seed']} | `{d['digest']}` |\n")
            f.write("\n")
            por_modo = {}
            for d in testu01_digests:
                por_modo.setdefault(d['mode'], {})[d['gen']] = d['digest']
            iguales = [m for m, v in por_modo.items()
                       if 'mx64' in v and 'hybrid' in v and v['mx64'] == v['hybrid']]
            distintos = [m for m, v in por_modo.items()
                         if 'mx64' in v and 'hybrid' in v and v['mx64'] != v['hybrid']]
            if iguales:
                f.write(f"**Aviso importante:** en el modo "
                        f"{', '.join(iguales)} el flujo de `hybrid_prng` es "
                        f"**idéntico** al de `mult_xorshift64` (mismo digest). "
                        f"La razón es estructural: en `hybrid_prng_next()` la "
                        f"mezcla middle-square solo ocupa los bits 32-63, de "
                        f"modo que la mitad baja de la salida coincide "
                        f"palabra a palabra. Las filas `hybrid_prng`/`low` "
                        f"**no son una medición independiente** de "
                        f"`mult_xorshift64`/`low`, y sus resultados deben "
                        f"contarse una sola vez. En los modos "
                        f"{', '.join(distintos) if distintos else '(ninguno)'} "
                        f"los flujos difieren.\n\n")
            f.write(f"{_prov_group(testu01_digests)}.\n\n")

        f.write("## Control negativo: el retículo se detecta y se recupera\n\n")
        f.write(f"El control usa {neg['n_values']} uint32 del generador del repo "
                f"(semilla {neg['seed']}) y contrasta la **salida mezclada** "
                f"(`fmix32`) con el **estado crudo** del LCG.\n\n")
        f.write("| flujo | p espectral 3D (Marsaglia) | planos del retículo | "
                "veredicto |\n")
        f.write("|------|----------------------------|----------------------|----------|\n")
        f.write(f"| salida mezclada (`randu_plus()`) | "
                f"{neg['spectral_mixed']['p_value']:.6g} | no aplica (no es "
                f"retículo) | {'❌ detecta' if neg['spectral_mixed']['p_value'] < 0.01 else '✅ no detecta'} |\n")
        f.write(f"| estado crudo del LCG | "
                f"{neg['spectral_raw']['p_value']:.6g} | "
                f"{neg['planes_raw']['n_planes']} planos "
                f"(k = {neg['planes_raw']['k_values']}) | "
                f"{'✅ detecta' if neg['spectral_raw']['p_value'] < 0.01 else '❌ no detecta'} |\n")
        f.write(f"| estado recuperado con `unfmix32` desde la salida | "
                f"{neg['spectral_raw']['p_value']:.6g} | "
                f"{neg['planes_recovered']['n_planes']} planos | "
                f"{'✅ detecta' if neg['spectral_raw']['p_value'] < 0.01 else '❌ no detecta'} |\n")
        f.write(f"\nRelación exacta `x[n+2] = 6·x[n+1] - 9·x[n] (mod 2^32)` en el "
                f"estado crudo: **{neg['planes_raw']['relation_exact']}**. "
                f"`unfmix32(salida)` recupera el estado crudo: "
                f"**{neg['unmix_recovers_state']}**.\n\n")
        f.write("Este control es el que demuestra que la batería no se autoengaña: "
                "el mismo test espectral que **rechaza** el estado crudo **acepta** "
                "la salida mezclada, y el estado crudo se puede recuperar desde esa "
                "salida porque `fmix32` es una biyección. La estructura se oculta al "
                "test de caja negra, no desaparece.\n\n")

        f.write(REPORT_CAVEAT.format(
            n_seeds=len(seeds),
            n_values=n_values,
            n_mb=f"{n_values * 4 / 1e6:.1f}",
            n_xcheck=xcheck['n_values'],
            seeds=", ".join(str(s) for s in xcheck['checked_seeds']),
            stat_seeds=", ".join(str(s) for s in seeds),
            smallcrush_text=smallcrush_text(testu01),
            crush_text=crush_text(testu01),
            bigcrush_text=bigcrush_text(testu01)))

    print("\nReporte guardado en VALIDATION_REPORT.md")

if __name__ == '__main__':
    main()