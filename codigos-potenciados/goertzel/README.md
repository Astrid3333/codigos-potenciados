# Goertzel - Formulaciones K, α, β e híbrida

Implementaciones del algoritmo de Goertzel con tres formulaciones numéricas
y una FFT radix-2 para comparación. Incluye código C compilado y una tool de
ayuda en Python.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `goertzel.h` | Declaraciones de las funciones Goertzel (K, α, β, híbrida) y FFT radix-2 |
| `goertzel.c` | Implementación en C + benchmark de precisión y timing |
| `goertzel_tool.py` | Tool de ayuda en Python (mismos algoritmos, referencia numpy) |
| `README.md` | Este archivo |

## Formulaciones

| Forma | Uso recomendado | Error típico (N=131072) |
|-------|-----------------|-------------------------|
| `K = 2·cos(w)` | Zona media, `π/4 ≤ w ≤ 3π/4` (cerca de π/2) | 3.5e-15 en π/2 |
| `α = 4·sin²(w/2)` | `w < π/4` (pequeño) | 1.7e-13 en bin 1 |
| `β = 4·cos²(w/2)` | `w > 3π/4` (cerca de π, espejo) | 2.5e-13 en bin 65535 |
| `híbrida` | α si `w < π/2`, β si `w ≥ π/2` | Combina α y β |

La forma α falla cerca de π por cancelación espejo con `2 + K`; la forma β
corrige eso. La forma K es óptima cerca de π/2 porque `cos(w) ≈ 0` y no hay
cancelación.

## Compilar y ejecutar (C)

```bash
gcc -std=c11 -O2 goertzel.c -o goertzel_bench -lm
./goertzel_bench
```

Salida de ejemplo (valores aproximados):

```
N = 131072, ruido gaussiano. Error relativo en magnitud vs suma directa (long double)
      k        omega      K=2cos       alpha        beta      hibrida
      1   4.79e-05    4.8e-08    1.7e-13    1.7e-13    1.7e-13
   1000   4.79e-02    4.9e-12    2.7e-14    2.7e-14    2.7e-14
  32768   1.57e+00    2.7e-14    6.4e-13    6.1e-14    6.1e-14
  65535   3.14e+00    4.3e-08    2.4e-07    2.5e-13    2.5e-13

Benchmark (C, -O2, 20 repeticiones, N = 131072):
     forma     ms/ejec   relativo
   K=2cos       0.334       1.00x
   alpha        0.341       1.02x
   beta         0.410       1.23x
   hibrida      ...         ...
   FFT rad2     4.570        --
1 FFT ~= 14 bins de Goertzel K
```

## Tool de ayuda (Python)

```bash
python3 goertzel_tool.py
```

O importar como módulo:

```python
from goertzel_tool import goertzel_K, goertzel_alpha, goertzel_beta, goertzel_hybrid, ref_magnitude, relative_error

c = goertzel_K(x, w)
mag = abs(c)
err = relative_error(mag, ref_magnitude(x, w))
```

**ADVERTENCIA:** Python interpretado no es comparable en velocidad con C
compilado ni con FFTW. Los tiempos de este script son solo indicativos del
costo del bucle interpretado, no del algoritmo en sí.

## Benchmarks de referencia

### C compilado (-O2, sin -ffast-math, N = 131072)

| Variante | ms/ejec | Relativo a K |
|----------|---------|--------------|
| Goertzel K | 0.334 | 1.00× |
| Goertzel α | 0.341 | 1.02× |
| Goertzel β | 0.410 | 1.23× |
| FFT radix-2 | 4.57 | — |

Equilibrio: 1 FFT ≈ 14 bins de Goertzel (coherente con log₂N = 17).
Debajo de ~10-14 bins conviene Goertzel; arriba conviene FFT.

### Octave (FFTW, N = 131072, bin k = 1000)

| Método | ms | vs FFT |
|--------|-----|--------|
| FFT (FFTW) | 1.505 | 1× |
| Goertzel for interpretado, 1 bin | 555.8 | 369× |
| Goertzel vectorizado (filter) | 2.380 | 1.58× |
| Bin directo (sum) | 6.477 | 4.30× |

FFTW gana desde el primer bin; equilibrio en Octave: 0.63 bins.

## Integración como tool MCP

Si un agente o tool MCP necesita invocar el algoritmo de Goertzel, puede
usar `goertzel_tool.py` como referencia o compilar `goertzel.c` como binario
invocable desde el entorno del MCP. Las cuatro funciones expuestas son:

- `goertzel_K(x, w)` — forma K
- `goertzel_alpha(x, w)` — forma α
- `goertzel_beta(x, w)` — forma β
- `goertzel_hybrid(x, w)` — forma híbrida

Todas reciben un arreglo de doubles `x` y una frecuencia angular `w`, y
devuelven un coeficiente complejo `{re, im}`.

## Referencia

Resultados experimentales detallados en `lista_codigos_obsoletos.md` (raíz del
repositorio) y en `goertzel_test.py` / `goertzel_test.m`.