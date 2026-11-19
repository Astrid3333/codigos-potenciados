# Informe de Validación randu_plus

**Semilla:** 12345
**Datos:** 105 MB (26214400 uint32)
**Fecha:** Mon Nov  2 12:51:23 AM -03 2026
**Git commit:** dec7ec2e23e6b422aeb24b80779df5747e2e7b83

## Resultados

| Test | p-value / ratio | Pass |
|------|-----------------|------|
| Spectral3D | ratio=1.004 | ✅ |
| BirthdaySpacings | 0.039832 | ✅ |
| Permutation | 0.962004 | ✅ |
| NIST_Frequency | 0.873591 | ✅ |
| NIST_BlockFreq | 0.992484 | ✅ |
| NIST_Runs | 0.637862 | ✅ |

**Total: 6/6 pasados**

## Qué prueba este informe y qué NO

Este informe mide **propiedades estadísticas del flujo de bits** de la salida
del generador (distribución, espectro, espaciado de cumpleaños, permutaciones,
frecuencia, frecuencia por bloques y rachas). Los 6 tests son **tests de
aleatoriedad estándar**: el mismo criterio con el que se evalúa un generador
pseudoaleatorio maduro.

**No es una validación del generador `randu_plus`.** En concreto:

- Estos tests **no pueden detectar** el retículo lineal de RANDU ni un periodo
  corto, porque son tests de primer orden sobre una muestra de 26 millones de
  valores. El periodo real del generador es **2^30** y el retículo es
  recuperable por la recurrencia `x[n+2] = 6·x[n+1] - 9·x[n]`; eso está
  documentado en `randu_plus.h`, que recomienda explícitamente `msws64/` o
  `hybrid_prng/` para uso real.
- `fmix32` (el post-proceso de MurmurHash) es una **biyección**: reordena los
  estados de RANDU pero no añade entropía ni rompe el retículo.
- El recuento 6/6 dice "el flujo **parece** aleatorio a estas escalas", no
  "el generador es criptográficamente seguro" ni "cumple ISO/IEC 23837".

Para las propiedades estructurales del generador (biyectividad, periodo,
recuperación del retículo) está el **test incluido en `randu_plus.c`**, que
incluye `randu_plus.h` y hace 8 comprobaciones con exit code: es distinto y
complementario a este informe.

## Nota de reproducibilidad

El informe se generó con la semilla 12345 sobre 105 MB (26 214 400 `uint32`)
en el commit `dec7ec2e`. Para reproducir: `python3 randu_plus_validator.py`.

**Aviso: el validador no usa `randu_plus.h`.** `generate_binary()` escribe al
vuelo un `randu_plus_bin.c` con una **copia embebida** del generador, y compila
esa copia en vez de incluir el header. Es una segunda fuente de verdad que puede
divergir del header sin que nada lo detecte.

Hoy esa copia es **equivalente al header para la semilla 12345** (impar), y eso
es lo que sostiene las cifras de este informe: verificado generando 1000 valores
con ambas implementaciones y comparando salida a salida, son idénticos.

Pero **no son equivalentes en general**: la copia embebida omite la línea
`*state |= 1u` con la que `randu_plus.h` fuerza la semilla impar. Con una
semilla par, ambas implementaciones divergen **desde la salida 0** (medido con
semillas 0, 2 y 100). El header además evita así el subperiodo degenerado de la
semilla cero, que la copia embebida sí reproduce.

Recomendación: que `generate_binary()` deje de escribir su propio generador y
compile un `.c` que incluya `randu_plus.h`, de modo que el informe mida
exactamente el generador del repo.

