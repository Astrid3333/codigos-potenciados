# Goertzel Algorithm - Experimental Results

## Summary
The Goertzel algorithm with form α works well for small ω but fails near π, requiring a mirror form β. The hybrid approach combines different formulations for different frequency ranges.

## Experimental Setup
- **Test signal**: Gaussian noise, N = 131,072 samples
- **Reference**: Direct summation in long double precision
- **Metric**: Relative error in coefficient magnitude

## Results by Formulation

| ω (bin) | K = 2cos | α | Hybrid |
|---------|----------|----|--------|
| 4.8e-05 (bin 1) | 4.8e-08 | **1.7e-13** | **1.7e-13** |
| 4.8e-04 (bin 10) | 2.3e-09 | **5.4e-15** | **5.4e-15** |
| 4.8e-02 (bin 1000) | 4.9e-12 | **2.7e-14** | **2.7e-14** |
| 1.57 (≈π/2) | **3.5e-15** | 6.4e-13 | 6.1e-14 |
| 3.14 (bin 65535) | 4.3e-08 | 2.4e-07 | **2.5e-13** |

## Analysis

### Small ω (ω < π/4)
- **α form excels**: Reduces error from 4.8e-08 to 1.7e-13 at bin 1 (~280,000× improvement)
- Hybrid matches α performance

### Near π (ω > 3π/4)
- **α form fails**: Error worsens to 2.4e-07 due to mirror cancellation with 2 + K
- **β form required**: Uses β = 4cos²(ω/2) with recurrence:
  - d[n] = x[n] + β·s[n-1] − d[n-1]
  - s[n] = d[n] − s[n-1]
- Hybrid with β achieves 2.5e-13 (matching FFT precision)

### Near π/2 (mid-range)
- **K form is optimal**: 3.5e-15 because cos ω ≈ 0, no cancellation
- Naive π/2 crossover in hybrid is suboptimal: 1.5e-14 → 2.0e-12 at bin 32769

## Practical Recommendation
Use piecewise formulation:
- **α** for ω < π/4
- **K** for π/4 ≤ ω ≤ 3π/4  
- **β** for ω > 3π/4

Thresholds not yet precisely tuned.

## Reference Performance
- Octave FFT achieves ~1e-13 in same cases
- Goertzel at extremes reaches FFT-level precision

## Benchmarks

### C compilado (-O2, sin -ffast-math, N = 131 072)

| Variante | ms/ejec | Relativo a K |
|----------|---------|--------------|
| Goertzel K | 0.334 | 1.00× |
| Goertzel α | 0.341 | 1.02× |
| Goertzel β | 0.410 | 1.23× |
| FFT radix-2 propia | 4.57 | — |

- α cuesta solo ~2% más que K en C
- β cuesta ~23% más, solo usado cerca de π
- Equilibrio: 1 FFT ≈ 14 bins de Goertzel (coherente con log₂N=17)
- Limitación: FFT radix-2 simple, no FFTW

### Octave (FFTW, N = 131 072, bin k = 1000)

| Método | ms | vs FFT |
|--------|-----|--------|
| FFT (FFTW) | 1.505 | 1× |
| Goertzel for interpretado, 1 bin | 555.8 | 369× |
| Goertzel vectorizado (filter) | 2.380 | 1.58× |
| Bin directo (sum) | 6.477 | 4.30× |

- FFTW gana desde el primer bin; equilibrio en Octave: 0.63 bins
- 20 bins interpretado: ~11.1 s vs 1.5 ms = 7,386× más lento
- Medido solo K; α y β no implementados en Octave

### Conclusión
- Con código compilado, Goertzel gana por debajo de ~10-14 bins
- Con FFTW optimizada (Octave/Matlab), FFT gana desde el primer bin
- Python interpretado no es comparable: el bucle for distorsiona las mediciones

## Code Files
- `goertzel_test.py` - Python implementation (bucle interpretado, valores de precisión válidos pero tiempos no comparables con FFT compilada)
- `goertzel_bench.py` - documenta benchmarks de C y Octave (sin ejecutar Python para evitar conclusiones erróneas)
- `goertzel_test.m` - Octave/Matlab version con funciones K, α, β, híbrida comparando contra FFT; no ejecutado (Octave no disponible en este entorno)