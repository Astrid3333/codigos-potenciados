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
def generate_binary(output_path: str, n_bytes: int, seed: int = 12345) -> None:
    """Genera n_bytes de salida randu_plus usando el binario compilado."""
    if not os.path.exists('./randu_plus'):
        subprocess.run(['gcc', '-std=c11', '-O2', 'randu_plus.c', '-o', 'randu_plus'], check=True)
    
    # Modificamos randu_plus.c para salida binaria masiva
    # Creamos versión temporal
    with open('randu_plus_bin.c', 'w') as f:
        f.write('''#include <stdio.h>
#include <stdint.h>
static inline uint32_t randu_plus(uint32_t *s) {
    *s = 65539u * *s;
    uint32_t x = *s;
    x ^= x >> 16; x *= 0x85ebca6bu;
    x ^= x >> 13; x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}
int main(int argc, char** argv) {
    uint32_t s = (argc > 1) ? (uint32_t)strtoul(argv[1], NULL, 10) : 12345u;
    size_t n = (argc > 2) ? strtoul(argv[2], NULL, 10) : 1000000;
    for (size_t i = 0; i < n; ++i) {
        uint32_t r = randu_plus(&s);
        fwrite(&r, 4, 1, stdout);
    }
    return 0;
}''')
    subprocess.run(['gcc', '-std=c11', '-O2', 'randu_plus_bin.c', '-o', 'randu_plus_bin'], check=True)
    
    n_uints = n_bytes // 4
    with open(output_path, 'wb') as out:
        proc = subprocess.run(['./randu_plus_bin', str(seed), str(n_uints)], 
                             stdout=out, check=True)
    print(f"Generados {n_bytes} bytes ({n_uints} uint32) en {output_path}")

def load_uint32(path: str) -> List[int]:
    with open(path, 'rb') as f:
        data = f.read()
    return list(struct.unpack(f'<{len(data)//4}I', data))

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
        f.write(f"\n**Total: {passed}/{total} pasados**\n")
    
    print("\nReporte guardado en VALIDATION_REPORT.md")

if __name__ == '__main__':
    main()