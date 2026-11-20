# Informe de Validación randu_plus

**Semillas:** 12345, 1, 3, 7, 4242, 65537, 99991, 2147483647 (8 semillas impares distintas)
**Muestra por semilla:** 1000000 uint32 (4.0 MB)
**Fecha:** Fri Nov 20 12:47:00 AM -03 2026
**Git commit:** 42421d2620829697ecbc3717d89a04b7f1ba579e

## Resultados (peor caso entre semillas)

| Test | metrica | peor caso | semilla | pass |
|------|---------|-----------|---------|------|
| Spectral3D | ratio | 0.982369 | 3 | ✅ |
| BirthdaySpacings | p_value | 0.149875 | 3 | ✅ |
| Permutation | p_value | 0.024850 | 3 | ✅ |
| NIST_Frequency | p_value | 0.034827 | 1 | ✅ |
| NIST_BlockFreq | p_value | 0.145194 | 65537 | ✅ |
| NIST_Runs | p_value | 0.255144 | 99991 | ✅ |
| HeaderCrossCheck | bit a bit | 1000 valores x 5 semillas x 2 headers | - | ✅ |
| HeaderSync | sha256 | 2 copias de randu_plus.h | - | ✅ |

**Total: 8/8 pasados**

## Valor de cada test en cada semilla

| Test | 12345 | 1 | 3 | 7 | 4242 | 65537 | 99991 | 2147483647 |
|---|---|---|---|---|---|---|---|---|
| Spectral3D | 1.0040 | 1.0003 | 0.9824 | 1.0003 | 1.0017 | 1.0040 | 0.9930 | 0.9937 |
| BirthdaySpacings | 0.2208 | 0.3101 | 0.1499 | 0.9521 | 0.4149 | 0.5293 | 0.5293 | 0.1499 |
| Permutation | 0.9620 | 0.3751 | 0.0249 | 0.2517 | 0.7218 | 0.7539 | 0.2955 | 0.0807 |
| NIST_Frequency | 0.8736 | 0.0348 | 0.8881 | 0.9133 | 0.2360 | 0.5241 | 0.7242 | 0.8552 |
| NIST_BlockFreq | 0.9925 | 0.4681 | 0.7258 | 0.4283 | 0.7363 | 0.1452 | 0.2497 | 0.8672 |
| NIST_Runs | 0.6379 | 0.3192 | 0.8740 | 0.7825 | 0.6816 | 0.6256 | 0.2551 | 0.5460 |

## Muestra realmente consumida por test

| Test | uint32 consumidos | bits |
|------|-------------------|------|
| Spectral3D | 150000 | - |
| BirthdaySpacings | 20000 | - |
| Permutation | 50000 | - |
| NIST_Frequency | 1000000 | 32000000 |
| NIST_BlockFreq | 100000 | 3200000 |
| NIST_Runs | 100000 | 3200000 |

## TestU01 SmallCrush

| modo | semilla | resultado | estadisticas fuera de rango |
|------|---------|-----------|------------------------------|
| fmix | 12345 | ✅ pasan todas | 0 de 15 |
| fmix | 1 | ✅ pasan todas | 0 de 15 |
| fmix | 3 | ✅ pasan todas | 0 de 15 |
| fmix | 7 | ✅ pasan todas | 0 de 15 |
| fmix | 4242 | ✅ pasan todas | 0 de 15 |
| fmix | 65537 | ✅ pasan todas | 0 de 15 |
| fmix | 99991 | ✅ pasan todas | 0 de 15 |
| fmix | 2147483647 | ✅ pasan todas | 0 de 15 |
| state | 12345 | ❌ falla | 14 de 15 |

Version: TestU01 1.2.3. Harness: `codigos-potenciados/testu01/run_randu_plus` (local, **no versionado** en este commit).

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
  `msws64/` o `hybrid_prng/` para uso real.
- `fmix32` (el post-proceso de MurmurHash) es una **biyección**: reordena los
  estados de RANDU pero no añade entropía ni rompe el retículo.
- El recuento dice "el flujo **parece** aleatorio a estas escalas", no "el
  generador es criptográficamente seguro" ni "cumple ISO/IEC 23837".

### Lo que aporta la batería de TestU01

SmallCrush (15 estadísticas) **sí detecta** el retículo cuando recibe el
**estado crudo** del LCG: en modo `state` fallan 14 de las 15 estadísticas con
p-value ~0. En modo `fmix` (la salida de `randu_plus()`) **pasan las 15** en
las 8 semillas medidas. Es decir: `fmix32` borra la estructura
*observable* en caja negra, pero no la elimina (fmix32 es invertible) y el
periodo sigue siendo 2^30. Para uso real, `msws64/` o `hybrid_prng/`.

El harness de TestU01 (`codigos-potenciados/testu01/`) **no está versionado**
en este commit: los números de esa sección se midieron con el binario local
`run_randu_plus`. Si el harness no está, el validador omite la sección en vez
de reportar un resultado inventado.

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
