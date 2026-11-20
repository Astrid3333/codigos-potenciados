# Informe de Validación randu_plus

**Semilla:** 12345
**Datos:** 105 MB (26214400 uint32)
**Fecha:** Thu Nov 19 10:39:55 PM -03 2026
**Git commit:** a565488b2ada6e612df8b963061c10cb977989aa

## Resultados

| Test | p-value / ratio | Pass |
|------|-----------------|------|
| Spectral3D | ratio=1.004 | ✅ |
| BirthdaySpacings | 0.039832 | ✅ |
| Permutation | 0.962004 | ✅ |
| NIST_Frequency | 0.873591 | ✅ |
| NIST_BlockFreq | 0.992484 | ✅ |
| NIST_Runs | 0.637862 | ✅ |
| HeaderCrossCheck | 1000 valores x 5 semillas | ✅ |

**Total: 7/7 pasados**

## Qué prueba este informe y qué NO

Este informe mide **propiedades estadísticas del flujo de bits** de la salida
del generador (distribución, espectro, espaciado de cumpleaños, permutaciones,
frecuencia, frecuencia por bloques y rachas). Los tests son **tests de
aleatoriedad estándar**: el mismo criterio con el que se evalúa un generador
pseudoaleatorio maduro.

**No es una validación del generador `randu_plus`.** En concreto:

- Estos tests **no pueden detectar** el retículo lineal de RANDU ni un periodo
  corto, porque son tests de primer orden sobre una muestra de 26214400
  valores. El periodo real del generador es **2^30** y el retículo es
  recuperable por la recurrencia `x[n+2] = 6·x[n+1] - 9·x[n]`; eso está
  documentado en `randu_plus.h`, que recomienda explícitamente `msws64/` o
  `hybrid_prng/` para uso real.
- `fmix32` (el post-proceso de MurmurHash) es una **biyección**: reordena los
  estados de RANDU pero no añade entropía ni rompe el retículo.
- El recuento dice "el flujo **parece** aleatorio a estas escalas", no "el
  generador es criptográficamente seguro" ni "cumple ISO/IEC 23837".

Para las propiedades estructurales del generador (biyectividad, periodo,
recuperación del retículo) está el **test incluido en `randu_plus.c`**, que
incluye `randu_plus.h` y hace 8 comprobaciones con exit code: es distinto y
complementario a este informe.

## Qué mide el cross-check contra el header

`generate_binary()` compila un `.c` temporal que **incluye `randu_plus.h`**
(con `-I` apuntando al directorio del repo), de modo que el flujo binario
validado es exactamente el generador del repo. El test `HeaderCrossCheck`
comprueba **bit a bit** los primeros 1000 valores contra una referencia
escrita a mano en Python, para las semillas 12345, 0, 2, 100, 4294967295, y comprueba además que
una semilla par produce la misma secuencia que su versión impar (el
`*state |= 1u` del header).

Si el header cambia, hay que actualizar la referencia en el mismo commit: si
no, este test falla y lo dice. Ese fallo es intencional: es la alarma que
impide que el informe vuelva a medir un generador que no sea el del repo.

## Nota de reproducibilidad

El informe se generó con la semilla 12345 sobre 105 MB (26214400
`uint32`). Para reproducir: `python3 randu_plus_validator.py`.
