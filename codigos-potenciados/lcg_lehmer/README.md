# lcg_lehmer - Análisis Espectral de LCG de Lehmer (1949)

Implementación en C del análisis espectral de generadores congruenciales lineales
(LCG) de Lehmer, con énfasis en RANDU (a=65539, m=2^31) y sus defectos históricos.

## Estructura

| Archivo | Contenido |
|---------|-----------|
| `lcg_lehmer.h` | Cabecera con estructuras y declaraciones |
| `lcg_lehmer.c` | Implementación: análisis espectral, prueba de período, test de planos, híbrido, TCR-LCG |
| `lcg_lehmer_test.c` | Test completo con RANDU, MINSTD, híbrido, TCR |
| `README.md` | Este archivo |

## Análisis incluido

### 1. Análisis espectral
Los puntos k-dimensionales (x_n, x_{n+1}, ..., x_{n+k-1}) de un LCG caen sobre
hiperplanos. La distancia mínima entre planos adyacentes mide la calidad del
generador. Para RANDU en 3D, los puntos caen en solo 15 planos (famoso defecto
descubierto por Marsaglia 1968).

### 2. Prueba de período
Verifica si el LCG alcanza su período máximo posible.

### 3. Híbrido LCG + msws64
Combina LCG con msws64 (Widynski 2017) mediante XOR para obtener buena
distribución sin depender de un solo mecanismo.

### 4. TCR-LCG
Teorema Chino del Resto aplicado a LCG: el contador se lleva como restos
independientes módulo primos coprimos, reconstruido vía CRT.

## Compilar y ejecutar

```bash
gcc -std=c11 -O2 lcg_lehmer.c lcg_lehmer_test.c -o lcg_lehmer_test -lm
./lcg_lehmer_test
```

## Resultado esperado

```
=================================================================
  ANÁLISIS ESPECTRAL DE LCG DE LEHMER (1949)
=================================================================

1. RANDU (a=65539=2^16+3, m=2^31)
   Multiplicador histórico defectuoso

   k=2: a=65539 m=2147483648 dim=2
     distancia_min = ...
     num_planes    = ...
     spectral_score= 0.XXXX (1.0 = ideal)
     es_malo       = SÍ
   ...
   Planos en 3D (test de red Marsaglia): 15
   Esperado: ~15 planos (RANDU es famoso por esto)

2. MINSTD (a=16807=7^5, m=2^31-1, primo de Mersenne)
   ...
```

## Integración con el resto del proyecto

- `codigos-potenciados/msws64/` — generador msws64 puro
- `codigos-potenciados/goertzel/` — algoritmo Goertzel K/α/β/híbrido
- `codigos-potenciados/randu_plus.c` — RANDU + fmix32

## Referencia

- J. von Neumann, "Various techniques used in connection with random digits" (1951)
- D. H. Lehmer, "Mathematical methods in large-scale computing units" (1949)
- G. Marsaglia, "Random numbers fall mainly in the planes" (1968)
- Park & Miller, "Random number generators: good ones are hard to find" (1988)