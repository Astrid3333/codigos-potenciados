# Informe de Validación randu_plus

**Estudio didáctico, no homologación.** Este informe mide el comportamiento estadístico de una demostración de por qué RANDU no es un buen generador (ver `randu_plus.h`). No certifica ningún estándar (ISO/IEC 23837, PractRand) ni calidad criptográfica. Para generar números de calidad, este repo recomienda `mult_xorshift64/`, `hybrid_prng/` o el híbrido LCG+mult_xorshift64 de `lcg_lehmer/`.

**Semillas:** 12345, 1, 3, 7, 4242, 65537, 99991, 2147483647 (8 semillas impares distintas)
**Muestra por semilla:** 1000000 uint32 (4.0 MB)
**Fecha:** Sat Nov 21 01:03:56 AM -03 2026
**Git commit:** 091e200ccd09560cbaaebb18ae3eb5f7bf7bb090

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
| smallcrush | fmix | 12345 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | fmix | 1 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | fmix | 3 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | fmix | 7 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | fmix | 4242 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | fmix | 65537 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | fmix | 99991 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | fmix | 2147483647 | ✅ pasan todas | 0 de 15 | sin fecha (cache) |
| smallcrush | state | 12345 | ❌ falla | 14 de 15 | sin fecha (cache) |
| crush | fmix | 12345 | ❌ falla | 6 de 144 | sin fecha (cache) |
| crush | state | 12345 | ❌ falla | 131 de 144 | sin fecha (cache) |

- Fallos en `smallcrush` modo `state`: BirthdaySpacings, Collision, Gap, SimpPoker, CouponCollector, MaxOft, WeightDistrib, MatrixRank, HammingIndep, RandomWalk1 H, RandomWalk1 M, RandomWalk1 J...

- Fallos en `crush` modo `fmix`: Gap, r = 0, Gap, r = 22, MaxOft, t = 5, MaxOft, t = 10, MaxOft, t = 20, MaxOft, t = 30

- Fallos en `crush` modo `state`: SerialOver, t = 2, SerialOver, t = 4, CollisionOver, t = 2, CollisionOver, t = 2, CollisionOver, t = 4, CollisionOver, t = 4, CollisionOver, t = 8, CollisionOver, t = 8, CollisionOver, t = 20, CollisionOver, t = 20, BirthdaySpacings, t = 2, BirthdaySpacings, t = 3...

Version: TestU01 1.2.3. Harness: `codigos-potenciados/testu01/run_randu_plus` (fuentes versionadas; el binario se construye con el `Makefile` de ese directorio).

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

SmallCrush (15 estadisticas) **si detecta** el reticulo cuando recibe el **estado crudo** del LCG: en modo `state` (semilla 12345) fallan 14 de las 15 estadisticas (BirthdaySpacings, Collision, Gap, SimpPoker, CouponCollector, MaxOft, ...) (dato reutilizado de la cache, sin fecha de medicion). En modo `fmix` (la salida de `randu_plus()`) **pasan todas** las 8 semillas medidas (8 de 8 sin ninguna estadistica fuera de rango) (dato reutilizado de la cache, sin fecha de medicion). Es decir: `fmix32` borra la estructura *observable* en caja negra a escala SmallCrush, pero no la elimina (fmix32 es invertible) y el periodo sigue siendo 2^30. Para uso real, `mult_xorshift64/` o `hybrid_prng/`.

La batería **Crush** (144 estadísticas, 10 veces más que SmallCrush) va más
allá; lo que mide esta corrida es lo siguiente:

modo `fmix` (semilla 12345): fallan 6 de 144 estadisticas (Gap, r = 0, Gap, r = 22, MaxOft, t = 5, MaxOft, t = 10, MaxOft, t = 20, MaxOft, t = 30) (dato reutilizado de la cache, sin fecha de medicion). modo `state` (semilla 12345): fallan 131 de 144 estadisticas (SerialOver, t = 2, SerialOver, t = 4, CollisionOver, t = 2, CollisionOver, t = 2, CollisionOver, t = 4, CollisionOver, t = 4, ...) (dato reutilizado de la cache, sin fecha de medicion). Es decir: la salida mezclada **tambien falla** a escala Crush (6 de 144), aunque mucho menos que el estado crudo (131 de 144); `fmix32` oculta el reticulo en caja negra a escala SmallCrush, no a escala Crush, y desde luego no lo elimina ni cambia el periodo (2^30).

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
