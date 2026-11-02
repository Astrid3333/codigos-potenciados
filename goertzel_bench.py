# Goertzel Benchmark - Datos de C compilado y Octave

# NO ejecutar este script para extraer conclusiones de rendimiento:
# Python interpretado no es comparable con C compilado ni con FFTW.

# --- Datos de C compilado (-O2, sin -ffast-math, N = 131 072) ---
# Variante       ms/ejec   Relativo a K
# Goertzel K     0.334     1.00x
# Goertzel α     0.341     1.02x
# Goertzel β     0.410     1.23x
# FFT radix-2    4.57      —
# Equilibrio: 1 FFT ≈ 14 bins de Goertzel (coherente con log2(131072) = 17)

# --- Datos de Octave (FFTW, N = 131 072, bin k = 1000) ---
# Método                         ms    vs FFT
# FFT (FFTW)                     1.505 1x
# Goertzel for interpretado 1 bin 555.8 369x
# Goertzel vectorizado (filter)  2.380 1.58x
# Bin directo (sum)              6.477 4.30x
# Equilibrio en Octave: 0.63 bins (FFTW gana desde el primer bin)
# Medido solo K; α y β no implementados en Octave

# Conclusión:
# - Código compilado: Goertzel gana por debajo de ~10-14 bins
# - FFTW optimizada: FFT gana desde el primer bin
# - Python interpretado: no comparable (bucle for distorsiona)

print("Ver comentarios arriba. Script no ejecutado para evitar conclusiones erróneas.")