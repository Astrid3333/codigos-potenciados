# Informe de Validación randu_plus

**Estudio didáctico, no homologación.** Este informe mide el comportamiento estadístico de una demostración de por qué RANDU no es un buen generador (ver `randu_plus.h`). No certifica ningún estándar (ISO/IEC 23837, PractRand) ni calidad criptográfica. Para generar números de calidad, este repo recomienda `mult_xorshift64/`, `hybrid_prng/` o el híbrido LCG+mult_xorshift64 de `lcg_lehmer/`.

**Semillas:** 12345, 1, 3, 7, 4242, 65537, 99991, 2147483647 (8 semillas impares distintas)
**Muestra por semilla:** 1000000 uint32 (4.0 MB)
**Fecha:** Sat Nov 21 02:45:29 AM -03 2026
**Git commit:** adf35dd425aadf621496d0537c764969769bbcd3

## Resultados (peor caso entre semillas)

| Test | metrica | peor caso | semilla | pass |
|------|---------|-----------|---------|------|
| Spectral3D | ratio | 0.982369 | 3 | ✅ |
| MarsagliaSpectral3D | p_value | 0.116664 | 4242 | ✅ |
| BirthdaySpacings | p_value | 0.149875 | 3 | ✅ |
| Permutation | p_value | 0.079405 | 3 | ✅ |
| NIST_Frequency | p_value | 0.034827 | 1 | ✅ |
| NIST_BlockFreq | p_value | 0.223425 | 65537 | ✅ |
| NIST_Runs | p_value | 0.255144 | 99991 | ✅ |
| HeaderCrossCheck | bit a bit | 1000 valores x 5 semillas x 2 headers | - | ✅ |
| HeaderSync | sha256 | 2 copias de randu_plus.h | - | ✅ |
| LatticeNegativeControl | p + planos | 600000 uint32, semilla 12345 | - | ✅ |

**Total: 10/10 pasados**

## Valor de cada test en cada semilla

| Test | 12345 | 1 | 3 | 7 | 4242 | 65537 | 99991 | 2147483647 |
|---|---|---|---|---|---|---|---|---|
| Spectral3D | 1.0040 | 1.0003 | 0.9824 | 1.0003 | 1.0017 | 1.0040 | 0.9930 | 0.9937 |
| MarsagliaSpectral3D | 0.6939 | 0.7241 | 0.5184 | 0.7478 | 0.1167 | 0.5787 | 0.3925 | 0.6540 |
| BirthdaySpacings | 0.2208 | 0.3101 | 0.1499 | 0.9521 | 0.4149 | 0.5293 | 0.5293 | 0.1499 |
| Permutation | 0.8912 | 0.4026 | 0.0794 | 0.3104 | 0.6533 | 0.6786 | 0.3440 | 0.1559 |
| NIST_Frequency | 0.8736 | 0.0348 | 0.8881 | 0.9133 | 0.2360 | 0.5241 | 0.7242 | 0.8552 |
| NIST_BlockFreq | 0.9560 | 0.4722 | 0.6595 | 0.4440 | 0.6677 | 0.2234 | 0.3118 | 0.7805 |
| NIST_Runs | 0.6379 | 0.3192 | 0.8740 | 0.7825 | 0.6816 | 0.6256 | 0.2551 | 0.5460 |

## Muestra realmente consumida por test

| Test | uint32 consumidos | bits |
|------|-------------------|------|
| Spectral3D | 150000 | - |
| MarsagliaSpectral3D | 600000 | 19200000 |
| BirthdaySpacings | 20000 | - |
| Permutation | 50000 | - |
| NIST_Frequency | 1000000 | 32000000 |
| NIST_BlockFreq | 100000 | 3200000 |
| NIST_Runs | 100000 | 3200000 |

## TestU01 (harness local)

| bateria | modo | semilla | resultado | estadisticas fuera de rango | medido |
|---------|------|---------|-----------|------------------------------|--------|
| smallcrush | fmix | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:14:49 (cache) |
| smallcrush | fmix | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:14:55 (cache) |
| smallcrush | fmix | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:15:01 (cache) |
| smallcrush | fmix | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:15:08 (cache) |
| smallcrush | fmix | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:15:14 (cache) |
| smallcrush | fmix | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:15:21 (cache) |
| smallcrush | fmix | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:15:27 (cache) |
| smallcrush | fmix | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 01:15:33 (cache) |
| smallcrush | state | 12345 | ❌ falla | 14 de 15 | 2026-11-21 01:15:39 (cache) |
| smallcrush | state | 1 | ❌ falla | 14 de 15 | 2026-11-21 01:15:45 (cache) |
| smallcrush | state | 3 | ❌ falla | 14 de 15 | 2026-11-21 01:15:51 (cache) |
| smallcrush | state | 7 | ❌ falla | 14 de 15 | 2026-11-21 01:15:57 (cache) |
| smallcrush | state | 4242 | ❌ falla | 14 de 15 | 2026-11-21 01:16:03 (cache) |
| smallcrush | state | 65537 | ❌ falla | 14 de 15 | 2026-11-21 01:16:09 (cache) |
| smallcrush | state | 99991 | ❌ falla | 14 de 15 | 2026-11-21 01:16:15 (cache) |
| smallcrush | state | 2147483647 | ❌ falla | 14 de 15 | 2026-11-21 01:16:21 (cache) |
| crush | fmix | 12345 | ❌ falla | 6 de 144 | 2026-11-21 01:47:25 (cache) |
| crush | state | 12345 | ❌ falla | 131 de 144 | 2026-11-21 02:15:23 (cache) |

- Fallos en `smallcrush` modo `state` (semillas 12345, 1, 3, 7, 4242, 65537, 99991, 2147483647): BirthdaySpacings, Collision, Gap, SimpPoker, CouponCollector, MaxOft, WeightDistrib, MatrixRank, HammingIndep, RandomWalk1 H, RandomWalk1 M, RandomWalk1 J...

- Fallos en `crush` modo `fmix` (semilla 12345): Gap, r = 0, Gap, r = 22, MaxOft, t = 5, MaxOft, t = 10, MaxOft, t = 20, MaxOft, t = 30

- Fallos en `crush` modo `state` (semilla 12345): SerialOver, t = 2, SerialOver, t = 4, CollisionOver, t = 2, CollisionOver, t = 2, CollisionOver, t = 4, CollisionOver, t = 4, CollisionOver, t = 8, CollisionOver, t = 8, CollisionOver, t = 20, CollisionOver, t = 20, BirthdaySpacings, t = 2, BirthdaySpacings, t = 3...

Version: TestU01 1.2.3. Harness: `codigos-potenciados/testu01/run_randu_plus` (fuentes versionadas; el binario se construye con el `Makefile` de ese directorio).

## TestU01 SmallCrush sobre los modulos de 64 bits

El harness `run_prng_modules` somete a SmallCrush los generadores de 64 bits del repositorio y `xoshiro256**` como referencia de calidad conocida. Los modos `low`, `high` y `split` son las tres formas de trocear la palabra de 64 bits en palabras de 32 bits (mitad baja, mitad alta, o las dos en orden natural); no son post-procesos del generador.

| generador | modo | semilla | resultado | estadisticas fuera de rango | medido |
|-----------|------|---------|-----------|------------------------------|--------|
| mult_xorshift64 | low | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:29:27 (cache) |
| mult_xorshift64 | low | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:29:33 (cache) |
| mult_xorshift64 | low | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:29:40 (cache) |
| mult_xorshift64 | low | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:29:47 (cache) |
| mult_xorshift64 | low | 4242 | ❌ falla | 1 de 15 | 2026-11-21 02:29:54 (cache) |
| mult_xorshift64 | low | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:01 (cache) |
| mult_xorshift64 | low | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:07 (cache) |
| mult_xorshift64 | low | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:14 (cache) |
| mult_xorshift64 | high | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:21 (cache) |
| mult_xorshift64 | high | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:28 (cache) |
| mult_xorshift64 | high | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:35 (cache) |
| mult_xorshift64 | high | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:42 (cache) |
| mult_xorshift64 | high | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:48 (cache) |
| mult_xorshift64 | high | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:30:55 (cache) |
| mult_xorshift64 | high | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:02 (cache) |
| mult_xorshift64 | high | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:09 (cache) |
| mult_xorshift64 | split | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:16 (cache) |
| mult_xorshift64 | split | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:22 (cache) |
| mult_xorshift64 | split | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:29 (cache) |
| mult_xorshift64 | split | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:35 (cache) |
| mult_xorshift64 | split | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:42 (cache) |
| mult_xorshift64 | split | 65537 | ❌ falla | 1 de 15 | 2026-11-21 02:31:48 (cache) |
| mult_xorshift64 | split | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:31:55 (cache) |
| mult_xorshift64 | split | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:32:02 (cache) |
| hybrid_prng | low | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:32:11 (cache) |
| hybrid_prng | low | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:32:20 (cache) |
| hybrid_prng | low | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:32:28 (cache) |
| hybrid_prng | low | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:32:37 (cache) |
| hybrid_prng | low | 4242 | ❌ falla | 1 de 15 | 2026-11-21 02:32:46 (cache) |
| hybrid_prng | low | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:32:55 (cache) |
| hybrid_prng | low | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:33:04 (cache) |
| hybrid_prng | low | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:33:13 (cache) |
| hybrid_prng | high | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:33:22 (cache) |
| hybrid_prng | high | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:33:31 (cache) |
| hybrid_prng | high | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:33:40 (cache) |
| hybrid_prng | high | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:33:49 (cache) |
| hybrid_prng | high | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:33:58 (cache) |
| hybrid_prng | high | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:34:07 (cache) |
| hybrid_prng | high | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:34:16 (cache) |
| hybrid_prng | high | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:34:25 (cache) |
| hybrid_prng | split | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:34:32 (cache) |
| hybrid_prng | split | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:34:40 (cache) |
| hybrid_prng | split | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:34:48 (cache) |
| hybrid_prng | split | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:34:55 (cache) |
| hybrid_prng | split | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:03 (cache) |
| hybrid_prng | split | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:10 (cache) |
| hybrid_prng | split | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:18 (cache) |
| hybrid_prng | split | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:25 (cache) |
| xoshiro256** (referencia) | low | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:32 (cache) |
| xoshiro256** (referencia) | low | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:38 (cache) |
| xoshiro256** (referencia) | low | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:45 (cache) |
| xoshiro256** (referencia) | low | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:52 (cache) |
| xoshiro256** (referencia) | low | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:35:58 (cache) |
| xoshiro256** (referencia) | low | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:05 (cache) |
| xoshiro256** (referencia) | low | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:12 (cache) |
| xoshiro256** (referencia) | low | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:18 (cache) |
| xoshiro256** (referencia) | high | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:24 (cache) |
| xoshiro256** (referencia) | high | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:31 (cache) |
| xoshiro256** (referencia) | high | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:37 (cache) |
| xoshiro256** (referencia) | high | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:44 (cache) |
| xoshiro256** (referencia) | high | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:50 (cache) |
| xoshiro256** (referencia) | high | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:36:57 (cache) |
| xoshiro256** (referencia) | high | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:03 (cache) |
| xoshiro256** (referencia) | high | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:09 (cache) |
| xoshiro256** (referencia) | split | 12345 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:15 (cache) |
| xoshiro256** (referencia) | split | 1 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:22 (cache) |
| xoshiro256** (referencia) | split | 3 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:28 (cache) |
| xoshiro256** (referencia) | split | 7 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:34 (cache) |
| xoshiro256** (referencia) | split | 4242 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:41 (cache) |
| xoshiro256** (referencia) | split | 65537 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:47 (cache) |
| xoshiro256** (referencia) | split | 99991 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:37:53 (cache) |
| xoshiro256** (referencia) | split | 2147483647 | ✅ pasan todas | 0 de 15 | 2026-11-21 02:38:00 (cache) |

Resumen: mult_xorshift64: 22 de 24 configuraciones sin ninguna estadistica fuera de rango (maximo 1 fuera de rango); hybrid_prng: 23 de 24 configuraciones sin ninguna estadistica fuera de rango (maximo 1 fuera de rango); xoshiro256** (referencia): 24 de 24 configuraciones sin ninguna estadistica fuera de rango (maximo 0 fuera de rango).  (medido el 2026-11-21 02:38:00, dato reutilizado de la cache).

**Cómo leer los fallos puntuales.** TestU01 marca una estadística como fuera de rango cuando su p-valor queda fuera de [0.001, 0.9990]. Con 15 estadísticas y 24 configuraciones por generador son 360 comprobaciones, de las que se espera que unas 0.7 caigan fuera de rango por azar. Los fallos que aparecen son **de una sola estadística y marginales** (p = 0.9991, a 0.0001 del umbral), no sistemáticos; el generador de referencia `xoshiro256**` no falla ninguno en las mismas 24 configuraciones.

### Flujos comparados (digest FNV-1a de 1.000.000 de salidas de 32 bits)

| generador | modo | semilla | digest |
|-----------|------|---------|--------|
| mult_xorshift64 | low | 12345 | `1d63fbd9ce5edc96` |
| hybrid_prng | low | 12345 | `1d63fbd9ce5edc96` |
| xoshiro256** (referencia) | low | 12345 | `f3d3867902078264` |
| mult_xorshift64 | high | 12345 | `e9fcb764c18a2c2f` |
| hybrid_prng | high | 12345 | `0d340876c046d06b` |
| xoshiro256** (referencia) | high | 12345 | `1fcf9f58d990d0f1` |
| mult_xorshift64 | split | 12345 | `cb18d4e799d20df4` |
| hybrid_prng | split | 12345 | `fce7a59bc8f6e7e4` |
| xoshiro256** (referencia) | split | 12345 | `e1d250eb7b8609e7` |

**Aviso importante:** en el modo low el flujo de `hybrid_prng` es **idéntico** al de `mult_xorshift64` (mismo digest). La razón es estructural: en `hybrid_prng_next()` la mezcla middle-square solo ocupa los bits 32-63, de modo que la mitad baja de la salida coincide palabra a palabra. Las filas `hybrid_prng`/`low` **no son una medición independiente** de `mult_xorshift64`/`low`, y sus resultados deben contarse una sola vez. En los modos high, split los flujos difieren.

 (medido el 2026-11-21 02:45:29).

## Control negativo: el retículo se detecta y se recupera

El control usa 600000 uint32 del generador del repo (semilla 12345) y contrasta la **salida mezclada** (`fmix32`) con el **estado crudo** del LCG.

| flujo | p espectral 3D (Marsaglia) | planos del retículo | veredicto |
|------|----------------------------|----------------------|----------|
| salida mezclada (`randu_plus()`) | 0.693457 | no aplica (no es retículo) | ✅ no detecta |
| estado crudo del LCG | 0 | 15 planos (k = [-9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5]) | ✅ detecta |
| estado recuperado con `unfmix32` desde la salida | 0 | 15 planos | ✅ detecta |

Relación exacta `x[n+2] = 6·x[n+1] - 9·x[n] (mod 2^32)` en el estado crudo: **True**. `unfmix32(salida)` recupera el estado crudo: **True**.

Este control es el que demuestra que la batería no se autoengaña: el mismo test espectral que **rechaza** el estado crudo **acepta** la salida mezclada, y el estado crudo se puede recuperar desde esa salida porque `fmix32` es una biyección. La estructura se oculta al test de caja negra, no desaparece.


## Qué prueba este informe y qué NO

Este informe mide **propiedades estadísticas del flujo de bits** de la salida
del generador (distribución, espectro, espaciado de cumpleaños, permutaciones,
frecuencia monobit, frecuencia por bloques y rachas), repetido sobre
**8 semillas** y con criterio de paso en el **peor caso**: el mínimo
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
  (peor caso entre las 8 semillas: 0.106); sobre el estado crudo da
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

SmallCrush (15 estadisticas) **si detecta** el reticulo cuando recibe el **estado crudo** del LCG: en modo `state` fallan 14 de las 15 estadisticas en las 8 semillas medidas (BirthdaySpacings, Collision, CouponCollector, Gap, HammingIndep, MatrixRank, MaxOft, RandomWalk1 C, ...) con p-value ~0 (medido el 2026-11-21 01:16:21, dato reutilizado de la cache). En modo `fmix` (la salida de `randu_plus()`) **pasan todas** las 8 semillas medidas (8 de 8 sin ninguna estadistica fuera de rango) (medido el 2026-11-21 01:15:33, dato reutilizado de la cache). Es decir: `fmix32` borra la estructura *observable* en caja negra a escala SmallCrush, pero no la elimina (fmix32 es invertible) y el periodo sigue siendo 2^30. Para uso real, `mult_xorshift64/` o `hybrid_prng/`.

La batería **Crush** (144 estadísticas, 10 veces más que SmallCrush) va más
allá; lo que mide esta corrida es lo siguiente:

modo `fmix` (semilla 12345): fallan 6 de 144 estadisticas (Gap, r = 0, Gap, r = 22, MaxOft, t = 5, MaxOft, t = 10, MaxOft, t = 20, MaxOft, t = 30) (medido el 2026-11-21 01:47:25, dato reutilizado de la cache). modo `state` (semilla 12345): fallan 131 de 144 estadisticas (SerialOver, t = 2, SerialOver, t = 4, CollisionOver, t = 2, CollisionOver, t = 2, CollisionOver, t = 4, CollisionOver, t = 4, ...) (medido el 2026-11-21 02:15:23, dato reutilizado de la cache). Es decir: la salida mezclada **tambien falla** a escala Crush (6 de 144), aunque mucho menos que el estado crudo (131 de 144); `fmix32` oculta el reticulo en caja negra a escala SmallCrush, no a escala Crush, y desde luego no lo elimina ni cambia el periodo (2^30).

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

El informe se genera con 1000000 uint32 por semilla (4.0 MB), que es lo
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
comprueba **bit a bit** los primeros 1000 valores contra una referencia
escrita a mano en Python, para las semillas 12345, 0, 2, 100, 4294967295, y comprueba además que
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
12345, 1, 3, 7, 4242, 65537, 99991, 2147483647, y la muestra por semilla es 1000000 uint32 (4.0 MB).
