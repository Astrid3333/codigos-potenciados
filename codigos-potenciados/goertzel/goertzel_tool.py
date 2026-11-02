#!/usr/bin/env python3
"""
goertzel_tool.py - Tool de ayuda para el algoritmo de Goertzel (K, alpha, beta, hibrida).

Proporciona funciones de referencia en Python y un modo benchmark para comparar
contra FFT. Los valores de precisión coinciden con la implementación en C
(codigos-potenciados/goertzel/goertzel.c).

ADVERTENCIA: Python interpretado no es comparable en velocidad con C compilado
ni con FFTW. Los tiempos de este script son solo indicativos del costo relativo
del bucle interpretado, no del algoritmo en si.

Uso como tool:
    from goertzel_tool import goertzel_K, goertzel_alpha, goertzel_beta, goertzel_hybrid, ref_magnitude
    c = goertzel_K(x, w)
    mag = abs(c)

Uso como script:
    python3 goertzel_tool.py
"""

from __future__ import annotations
import math
import numpy as np
from typing import Tuple


def goertzel_K(x, w: float) -> complex:
    """Forma K = 2*cos(w). Mejor en zona media (cerca de pi/2)."""
    K = 2.0 * math.cos(w)
    s1 = s2 = 0.0
    for v in x:
        s = v + K * s1 - s2
        s2 = s1
        s1 = s
    s = K * s1 - s2  # x[N] = 0
    return s - complex(math.cos(w), -math.sin(w)) * s1


def goertzel_alpha(x, w: float) -> complex:
    """Forma alpha = 4*sin^2(w/2). Mejor para w pequeno (w < pi/4)."""
    al = 4.0 * math.sin(w / 2.0) ** 2
    s = d = 0.0
    for v in x:
        d = d + v - al * s
        s = s + d
    d = d - al * s
    s = s + d  # x[N] = 0
    s1 = s - d
    return s - complex(math.cos(w), -math.sin(w)) * s1


def goertzel_beta(x, w: float) -> complex:
    """Forma beta = 4*cos^2(w/2) = 2 + K. Espejo para w > pi/2 (cerca de pi)."""
    be = 4.0 * math.cos(w / 2.0) ** 2
    s = d = 0.0
    for v in x:
        d = v + be * s - d
        s = d - s
    d = be * s - d
    s = d - s  # x[N] = 0
    s1 = d - s
    return s - complex(math.cos(w), -math.sin(w)) * s1


def goertzel_hybrid(x, w: float) -> complex:
    """Hibrida: alpha si w < pi/2, beta si w >= pi/2."""
    if w < math.pi / 2.0:
        return goertzel_alpha(x, w)
    return goertzel_beta(x, w)


def ref_magnitude(x, w: float) -> float:
    """Referencia: suma directa en long double (numpy longdouble)."""
    n = np.arange(len(x), dtype=np.longdouble)
    R = np.sum(x.astype(np.longdouble) * np.exp(-1j * np.longdouble(w) * n))
    return abs(R)


def relative_error(mag_goertzel: float, mag_ref: float) -> float:
    return abs(mag_goertzel - mag_ref) / abs(mag_ref)


def run_benchmark(N: int = 2 ** 17, seed: int = 1) -> None:
    """Ejecuta el test de precision reproduciendo los resultados del paper."""
    rng = np.random.default_rng(seed)
    x = rng.standard_normal(N)
    X = np.fft.fft(x)

    print(f"N = {N}, ruido gaussiano. Error relativo vs suma directa (long double)\n")
    print(f"{'bin k':>7} {'omega':>12} {'K=2cos':>12} {'alpha':>12} {'beta':>12} {'hibrida':>12} {'fft':>12}")

    bins = [1, 3, 10, 100, 1000, N // 8, N // 4, N // 2 - 1]
    for k in bins:
        w = 2 * math.pi * k / N
        R = ref_magnitude(x, w)
        eK = relative_error(abs(goertzel_K(x, w)), R)
        eA = relative_error(abs(goertzel_alpha(x, w)), R)
        eB = relative_error(abs(goertzel_beta(x, w)), R)
        eH = relative_error(abs(goertzel_hybrid(x, w)), R)
        eF = relative_error(abs(X[k]), R)
        print(f"{k:7d} {w:12.2e} {eK:12.1e} {eA:12.1e} {eB:12.1e} {eH:12.1e} {eF:12.1e}")

    print("\nVariant hibrida (alpha si w<pi/2, beta si w>=pi/2):")
    print(f"{'bin k':>7} {'omega':>12} {'K=2cos':>12} {'hibrida':>12}")
    bins2 = [1, 10, 1000, N // 4 - 1, N // 4 + 1, N // 2 - 1000, N // 2 - 10, N // 2 - 1]
    for k in bins2:
        w = 2 * math.pi * k / N
        R = ref_magnitude(x, w)
        eK = relative_error(abs(goertzel_K(x, w)), R)
        eH = relative_error(abs(goertzel_hybrid(x, w)), R)
        print(f"{k:7d} {w:12.2e} {eK:12.1e} {eH:12.1e}")


if __name__ == "__main__":
    run_benchmark()