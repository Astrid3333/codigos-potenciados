# Goertzel - Formulaciones K, α, β e híbrida

Implementaciones del algoritmo de Goertzel con tres formulaciones numéricas
y una FFT radix-2 para comparación. Incluye código C compilado y una tool de
ayuda en Python.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `goertzel.h` | Declaraciones de las funciones Goertzel (K, α, β, híbrida) y de la FFT radix-2 |
| `goertzel.c` | Implementación en C de las cuatro formulaciones y de la FFT radix-2 in-place |
| `goertzel_test.c` | Demo: tones en bins específicos con las tres formulaciones |
| `goertzel_vs_fft_test.c` | Pruebas: Goertzel contra FFT radix-2 y contra la magnitud analítica |
| `goertzel_tool.py` | Tool de ayuda en Python (mismos algoritmos, referencia numpy) |
| `README.md` | Este archivo |

En la raíz del repositorio hay tres scripts de referencia que no forman parte
del módulo: `goertzel_test.py` (Python+numpy: error relativo contra la suma
directa en `long double`), `goertzel_test.m` (Octave: FFTW vs Goertzel) y
`goertzel_bench.py` (datos de rendimiento ya medidos, comentados; no ejecutar
para extraer conclusiones de rendimiento).

## Formulaciones

| Forma | Uso recomendado | Error típico (N=131072) |
|-------|-----------------|-------------------------|
| `K = 2·cos(w)` | Zona media, `π/4 ≤ w ≤ 3π/4` (cerca de π/2) | 1.2e-13 en π/2 |
| `α = 4·sin²(w/2)` | `w < π/4` (pequeño) | 4.7e-13 en bin 1 |
| `β = 4·cos²(w/2)` | `w > 3π/4` (cerca de π, espejo) | 3.8e-13 en bin 65535 |
| `híbrida` | α si `w < π/4`, K si `π/4 ≤ w ≤ 3π/4`, β si `w > 3π/4` | La mejor en cada zona |

La forma α falla cerca de π por cancelación espejo con `2 + K`; la forma β
corrige eso. La forma K es óptima cerca de π/2 porque `cos(w) ≈ 0` y no hay
cancelación.

La híbrida usa **3 zonas**, no 2: en `w = π/2` exacto las formas α y β dan
1.6e-11 mientras K da 1.2e-13, así que un corte en π/2 (que usaría β justo
en el centro) es subóptimo. Además pliega por aliasing (`w' = 2π - w`, conjugando
el resultado) para `w > π`. Con el plegado, el peor error relativo en `[0, 2π)`
baja de 2.2e-09 a 1.4e-10. Los umbrales π/4 y 3π/4 están medidos, no supuestos:
ver `resultados_goertzel.md` en la raíz del repositorio.

## Compilar y ejecutar (C)

`goertzel.c` es una biblioteca (no tiene `main`): hay que linkearla con uno de
los dos programas del módulo.

Suite de pruebas (Goertzel contra FFT):

```bash
gcc -std=c11 -O2 goertzel.c goertzel_vs_fft_test.c -lm -o prueba_fft
./prueba_fft
```

Demo de tonos (sin asserts, imprime magnitudes):

```bash
gcc -std=c11 -O2 goertzel.c goertzel_test.c -lm -o demo_goertzel
./demo_goertzel
```

Salida real de la demo (N = 131072, ruido gaussiano con 5 tonos):

```
N = 131072, ruido gaussiano con 5 tonos

    bin        omega         K=2cos        alpha      hibrida
      1     0.000048   65535.999523   65536.000000   65536.000000
     10     0.000479   32768.000026   32768.000000   32768.000000
   1000     0.047937   16384.000000   16384.000000   16384.000000
  32768     1.570796   32768.000000   32768.000000   32768.000000
  65535     3.141545   16383.999886   16384.000672   16384.000000
```

La tabla de error relativo en magnitud (contra la suma directa en `long double`)
la produce el script de referencia de la raíz del repositorio:

```bash
python3 ../../goertzel_test.py
```

```
N = 131072, entrada ruido gaussiano. Error relativo en magnitud vs suma directa en long double

  bin k      omega     K=2cos      alpha        fft
      1   4.79e-05    4.8e-08    1.7e-13    2.5e-16
      3   1.44e-04    6.6e-09    1.2e-13    3.9e-16
     10   4.79e-04    2.3e-09    5.4e-15    4.3e-16
    100   4.79e-03    1.5e-10    9.2e-15    3.9e-16
   1000   4.79e-02    4.9e-12    2.7e-14    4.5e-14
  16384   7.85e-01    2.3e-13    2.1e-13    1.8e-13
  32768   1.57e+00    2.7e-14    6.4e-13    4.3e-13
  65535   3.14e+00    4.3e-08    2.4e-07    1.8e-12

Variante hibrida (3 zonas: alpha si w<pi/4, K si pi/4<=w<=3pi/4, beta si w>3pi/4,
con plegado por aliasing 2pi-w para w>pi):
  bin k      omega     K=2cos    hibrida
      1   4.79e-05    4.8e-08    1.7e-13
     10   4.79e-04    2.3e-09    5.4e-15
   1000   4.79e-02    4.9e-12    2.7e-14
  32767   1.57e+00    3.5e-15    3.5e-15
  32769   1.57e+00    1.5e-14    1.5e-14
  64536   3.09e+00    7.2e-13    1.5e-15
  65526   3.14e+00    6.5e-10    1.9e-14
  65535   3.14e+00    4.3e-08    2.5e-13
```

## FFT radix-2 in-place

`goertzel.c` incluye una FFT radix-2 iterativa, in-place, sobre arrays de
parte real e imaginaria:

```c
int    goertzel_fft(double *re, double *im, size_t n);   /* 0 si ok, -1 si n no es potencia de dos */
double goertzel_fft_mag(const double *re, const double *im, size_t n, size_t k);
int    goertzel_es_potencia_de_dos(size_t n);
```

La salida es la DFT **sin normalizar**, de modo que `|X[k]|` es directamente
comparable con `|goertzel_K(x, n, 2*pi*k/n)|`: las dos rutas dan la misma
magnitud (no hace falta dividir por N).

## Pruebas: Goertzel contra FFT

```bash
gcc -std=c11 -O2 goertzel.c goertzel_vs_fft_test.c -lm -o prueba_fft
./prueba_fft
```

La suite verifica 24 casos: la FFT contra la DFT directa, el rechazo de
longitudes que no son potencia de dos, y la coincidencia de Goertzel K, α, β y
FFT contra la magnitud analítica `N*A/2` de tonos puros (N = 8192, bins 1,
1000 y 4095).

Resultados medidos (salida real de la suite):

```
   bin      analitico            FFT     Goertzel K     Goertzel a     Goertzel b
     1    4096.000000    4096.000000    4096.000000    4096.000000    4095.999999  (spread 5.057e-07)
  1000    2048.000000    2048.000000    2048.000000    2048.000000    2048.000000  (spread 4.434e-11)
  4095    1024.000000    1024.000000    1024.000000    1024.000000    1024.000000  (spread 1.301e-07)
```

Error máximo de la FFT contra la DFT directa (N = 64): 5.048e-13.

La suite compila limpia con los flags estrictos del proyecto y cierra con
`29/29 pruebas pasaron`:

```bash
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
    -Wsign-conversion -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes \
    -Werror goertzel.c goertzel_vs_fft_test.c -lm -o prueba_fft
```

Dos comentarios sobre las tolerancias:

- El spread entre variantes se evalúa **relativo** (spread / magnitud), no
  absoluto: con magnitudes del orden de 10³, un spread absoluto de 1e-8 es
  más exigente que la precisión de `double` en las formas menos precisas.
- El spread absoluto medido (5.1e-07 en el bin 1, 1.3e-07 en el bin 4095) lo
  aporta siempre la variante que no corresponde a esa zona: β se degrada
  cuando ω es pequeño y K cuando ω está cerca de π, tal como describe la tabla
  de formulaciones.

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

Datos ya medidos y archivados en `goertzel_bench.py` (raíz del repositorio);
el binario que los produjo no forma parte del repo, así que conviene volver a
medir si se los quiere comparar con hardware nuevo.

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

Resultados experimentales detallados y umbrales medidos en `resultados_goertzel.md`
(raíz del repositorio), y en la salida de `goertzel_test.c` / `goertzel_vs_fft_test.c`.