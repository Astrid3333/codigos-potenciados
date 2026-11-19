# Goertzel — resultados experimentales y umbrales de formulación

Este archivo reemplaza a `lista_codigos_obsoletos.md`, cuyo nombre no correspondía
a su contenido: no es una lista de códigos obsoletos, sino el registro de
mediciones del algoritmo de Goertzel. Se renombró para que el nombre diga lo que
el archivo contiene.

## Resumen

El algoritmo de Goertzel tiene tres formulaciones algebraicamente equivalentes,
pero con comportamiento numérico distinto:

| Forma | Coeficiente | Zona de uso | Error típico (N=131072) |
|-------|-------------|-------------|-------------------------|
| `K = 2·cos(w)` | original | zona media, `π/4 ≤ w ≤ 3π/4` | 1.2e-13 en `w = π/2` |
| `α = 4·sin²(w/2)` | reescritura | `w < π/4` | 4.7e-13 en bin 1 |
| `β = 4·cos²(w/2)` | espejo de α | `w > 3π/4` | 3.8e-13 en bin 65535 |

La forma K pierde precisión decimal cuando `w → 0` (porque `2·cos(w) → 2` y el
coeficiente deja de representar bien la diferencia entre raíces contiguas) y por
simetría cuando `w → π`. Las formas α y β reescriben el coeficiente para que no
haya cancelación, y por eso son las buenas en los extremos.

## Montaje experimental

- **Señal**: ruido **uniforme** en [-1, 1] generado con un LCG de 64 bits
  (`st = st·6364136223846793005 + 1442695040888963407`), N = 131,072 muestras,
  4 realizaciones con semillas fijas. Las tablas de este archivo salen de los
  programas en C con esa señal; las del README del módulo salen de
  `goertzel_test.py`, que usa ruido **gaussiano** (`numpy.random.default_rng(1)`).
  Por eso los números de un sitio y otro no coinciden exactamente: son
  mediciones distintas, cada una reproducible con su propio script.
- **Referencia**: suma directa en `long double` (`cosl`/`sinl`), no FFT, para que
  la referencia no dependa del algoritmo que se quiere medir
- **Métrica**: error relativo de la magnitud del coeficiente
- **Reproducir**: `goertzel_umbrales.c` (barrido de `w` y mejor forma por punto)
  y `goertzel_umbrales_hibrida.c` (comparación de híbridas). Compilar con
  `cc -std=c99 -O2 goertzel_umbrales.c -o umbrales -lm`.

## Resultados por formulación

Medidos con N = 131072 y referencia `long double` (semilla fija, 4 realizaciones):

| Bin | `w` | K = 2cos | α | β | mejor |
|-----|-----|----------|---|---|-------|
| 1 | 4.79e-05 | 3.9e-06 | **4.7e-13** | 2.2e-05 | α |
| 10 | 4.79e-04 | 3.9e-08 | **1.9e-14** | 6.8e-08 | α |
| 100 | 4.79e-03 | 1.4e-10 | **1.2e-14** | 1.3e-10 | α |
| 1000 | 4.79e-02 | 5.1e-12 | **4.0e-14** | 5.0e-11 | α |
| 4096 | 1.96e-01 | 1.1e-14 | **1.0e-14** | 5.9e-13 | α |
| 16384 (= π/4) | 7.85e-01 | 2.0e-12 | **2.2e-12** | 6.2e-12 | α/K (empate) |
| 32768 (= π/2) | 1.57e+00 | **1.2e-13** | 1.6e-11 | 1.6e-11 | K |
| 32769 | 1.57e+00 | 3.2e-13 | **2.1e-13** | 1.8e-12 | α |
| 49152 (= 3π/4) | 2.36e+00 | **8.9e-14** | 9.0e-14 | 1.7e-12 | K/α |
| 61440 | 2.95e+00 | 5.4e-11 | 2.0e-10 | **1.2e-12** | β |
| 65535 | 3.14e+00 | 1.7e-08 | 9.4e-08 | **3.8e-13** | β |

## Análisis

### ω pequeño (ω < π/4)

La forma α reduce el error desde 3.9e-06 hasta 4.7e-13 en el bin 1 (unas
8,000 veces mejor). La causa es que `K = 2·cos(w)` vale `2 - w²` en coma
flotante, y para `w` pequeño los bits de `w²` se pierden; la forma α usa
`α = 4·sin²(w/2)`, que sí conserva esos bits.

### Cerca de π (ω > 3π/4)

La forma α falla por la misma razón especular: `cos(w) → -1`. La forma β
usa `β = 4·cos²(w/2)` y recupera la precisión (3.8e-13 en el bin 65535 frente
a 1.7e-08 de K).

### Cerca de π/2 (zona media)

K es la más estable: `cos(w) ≈ 0` y no hay cancelación. En `w = π/2` exacto,
α y β dan 1.6e-11 mientras K da 1.2e-13 — **dos órdenes de magnitud de
diferencia**. Esto es lo que hace que una híbrida con corte en π/2 sea
subóptima: justo en el corte usa β, la peor de las tres.

## Recomendación práctica (umbrales medidos)

Formulación por tramos, con los umbrales afinados por medición y no supuestos:

- **α** para `w < π/4`
- **K** para `π/4 ≤ w ≤ 3π/4`
- **β** para `w > 3π/4`

Los umbrales π/4 y 3π/4 no son arbitrarios: son los puntos donde la forma K
empieza a ser competitiva con α (respectivamente con β), y coinciden con la
recomendación piecewise del informe original.

### Plegado por aliasing

Para `w > π` hay que plegar: `X[2πk/N]` es la conjugada de `X[2π(N-k)/N]`
para señal real. Se calcula en `w' = 2π - w` y se conjuga el resultado.

**Por qué hace falta y qué mejora exactamente.** El problema no es que β sea
"la peor forma" en toda la zona `w > π` — de hecho, β es la **mejor** cerca de
π (3.8e-13 en el bin 65535, frente a 1.7e-08 de K). El problema aparece en el
extremo `w → 2π`: allí `cos(w) → 1` y las dos formas que derivan de él
degeneran a la vez, `K → 2` y `β = 4·cos²(w/2) → 4`, que es el mismo
coeficiente degenerado que K. La forma α (`4·sin²(w/2)`) no degenera: vale
`α → 0` cuando `w → 0`. Plegar `w → 2π - w` lleva el extremo `w → 2π` al
extremo `w' → 0`, donde la híbrida elige α, y de ahí la mejora.

Medido sobre la rejilla `w ∈ [0, 2π]` con 120 pasos (N = 131072, 4
realizaciones):

| Híbrida | Peor error | Peor en `w` | Error medio |
|---------|------------|-------------|--------------|
| 2 zonas (corte en π/2, la original) | 2.215e-09 | 2π (w/π = 2.00) | 1.312e-11 |
| 3 zonas sin plegar | 2.215e-09 | 2π (w/π = 2.00) | 1.281e-11 |
| **3 zonas con plegado `2π-w`** | **1.424e-10** | 3.822 (w/π = 1.22) | **4.557e-12** |

El plegado baja el peor caso un factor **15.6** y el error medio un factor
**2.9**. El cambio de 2 a 3 zonas, por sí solo, apenas mueve el error medio
(1.312e-11 → 1.281e-11): su aporte real es **puntual**, en las bandas
intermedias donde la 2 zonas usaba α o β en vez de K (ver la sección de π/2).

Reproducir con `cc -std=c99 -O2 goertzel_umbrales_hibrida.c -o umbrales_h -lm`.

## Verificación de equivalencia

Las tres implementaciones (C, Python, Octave) de la híbrida de 3 zonas con
plegado producen **resultados idénticos bit a bit** sobre la misma señal de 512
muestras (14 bins de prueba, diferencia máxima 0.000e+00 en parte real e
imaginaria).

## Rendimiento

### C compilado (-O2, sin -ffast-math, N = 131072)

| Variante | ms/ejec | Relativo a K |
|----------|---------|--------------|
| Goertzel K | 0.334 | 1.00× |
| Goertzel α | 0.341 | 1.02× |
| Goertzel β | 0.410 | 1.23× |
| FFT radix-2 | 4.57 | — |

- α cuesta solo ~2% más que K en C
- β cuesta ~23% más, solo se usa cerca de π
- Equilibrio: 1 FFT ≈ 14 bins de Goertzel (coherente con log₂N = 17)

### Octave (FFTW, N = 131072, bin k = 1000)

| Método | ms | vs FFT |
|--------|-----|--------|
| FFT (FFTW) | 1.505 | 1× |
| Goertzel for interpretado, 1 bin | 555.8 | 369× |
| Goertzel vectorizado (filter) | 2.380 | 1.58× |
| Bin directo (sum) | 6.477 | 4.30× |

- FFTW gana desde el primer bin; equilibrio en Octave: 0.63 bins
- 20 bins interpretado: ~11.1 s vs 1.5 ms = 7,386× más lento
- Python interpretado no es comparable: el bucle `for` distorsiona las mediciones

## Nota sobre `goertzel_test.m`

El script en la raíz del repositorio **sí** se ejecutó (Octave está disponible
en este entorno). Al hacerlo se encontró y corrigió un bug de iteración: en
Octave `for v = x` itera por **columnas**, no por elementos. Con `x` de 1×N eso
recorría una sola vez con `v` = la fila entera, aplicando la recurrencia sobre
un vector en vez de sobre la serie temporal. Ahora usa `for idx = 1:numel(x)`.

Con la corrección, la híbrida de 3 zonas gana (o empata) en las 12 frecuencias
probadas, incluidas las de `w > π`.

## Archivos de código

- `codigos-potenciados/goertzel/goertzel.c` — implementación C (K, α, β, híbrida de 3 zonas con plegado)
- `codigos-potenciados/goertzel/goertzel_tool.py` — tool Python con las mismas cuatro funciones
- `goertzel_test.m` — versión Octave/Matlab, ejecutada y verificada
- `goertzel_umbrales.c` — barrido de `w` para localizar la mejor forma por punto
- `goertzel_umbrales_hibrida.c` — comparación de híbridas de 2 y 3 zonas
- `codigos-potenciados/goertzel/goertzel_vs_fft_test.c` — 29 pruebas contra FFT (todas pasan)
- `goertzel_bench.py` — documenta benchmarks de C y Octave (sin ejecutar Python para evitar conclusiones erróneas)
