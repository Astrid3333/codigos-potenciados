# Bateria estadistica de los PRNG de 64 bits del repo

**Estudio, no homologacion.** Mide el flujo de bits de `msws64`, `hybrid_prng` y del híbrido LCG+msws64 con la misma bateria que usa `randu_plus_validator.py`, y la acompaña de dos controles: uno limpio (`/dev/urandom`, que debe comportarse como un generador ideal) y uno defectuoso (el estado crudo de `randu_plus`, que debe fallar). No es TestU01 ni PractRand: TestU01 real no esta instalado en este entorno y el harness versionado solo cubre `randu_plus`.

**Semillas:** 12345, 1, 3, 7, 4242, 65537, 99991, 2147483647 (8 semillas)
**Muestra por semilla:** 500000 uint64 = 1000000 uint32 (palabras de 32 bits NO solapadas, en el orden de bytes que ve un consumidor)
**Comparaciones:** 56 p-values por generador (7 tests x 8 semillas)
**Criterio:** con 56 p-values y umbral 0.01, un flujo aleatorio espera 0.56 falsos positivos; el limite de 99.9% es 4. Se exige ademas cero p-values por debajo de 0.001 (senal fuerte) y que el numero de p-values < 0.01 no supere el del control limpio por mas de 1.

## Resultados (peor caso entre 8 semillas)

| Generador | Test | metrica | peor caso | semilla | pass |
|---|---|---|---|---|---|
| msws64 | Spectral3D | ratio | 0.977499 | 7 | ✅ |
| msws64 | MarsagliaSpectral3D | p_value | 0.162391 | 1 | ✅ |
| msws64 | BirthdaySpacings | p_value | 0.220815 | 2147483647 | ✅ |
| msws64 | Permutation | p_value | 0.191703 | 4242 | ✅ |
| msws64 | NIST_Frequency | p_value | 0.0342533 | 2147483647 | ✅ |
| msws64 | NIST_BlockFreq | p_value | 0.010525 | 65537 | ✅ |
| msws64 | NIST_Runs | p_value | 0.140598 | 65537 | ✅ |
| hybrid_prng | Spectral3D | ratio | 0.979167 | 7 | ✅ |
| hybrid_prng | MarsagliaSpectral3D | p_value | 0.141945 | 65537 | ✅ |
| hybrid_prng | BirthdaySpacings | p_value | 0.414924 | 3 | ✅ |
| hybrid_prng | Permutation | p_value | 0.297366 | 3 | ✅ |
| hybrid_prng | NIST_Frequency | p_value | 0.125448 | 2147483647 | ✅ |
| hybrid_prng | NIST_BlockFreq | p_value | 0.00757203 | 65537 | ❌ |
| hybrid_prng | NIST_Runs | p_value | 0.158578 | 1 | ✅ |
| lcg_msws64 | Spectral3D | ratio | 0.977583 | 7 | ✅ |
| lcg_msws64 | MarsagliaSpectral3D | p_value | 0.0751181 | 3 | ✅ |
| lcg_msws64 | BirthdaySpacings | p_value | 0.097023 | 99991 | ✅ |
| lcg_msws64 | Permutation | p_value | 0.0441994 | 65537 | ✅ |
| lcg_msws64 | NIST_Frequency | p_value | 0.0342533 | 2147483647 | ✅ |
| lcg_msws64 | NIST_BlockFreq | p_value | 0.127585 | 12345 | ✅ |
| lcg_msws64 | NIST_Runs | p_value | 0.249807 | 2147483647 | ✅ |

## Conteo de p-values por debajo del umbral

| Flujo | p < 0.01 | p < 0.001 | comparaciones | veredicto |
|---|---|---|---|---|
| /dev/urandom (control limpio) | 1 | 0 | 56 | referencia |
| msws64 | 0 | 0 | 56 | ✅ sin señal |
| hybrid_prng | 1 | 0 | 56 | ✅ sin señal |
| lcg_msws64 | 0 | 0 | 56 | ✅ sin señal |

### Detalle de cada p-value por debajo de 0.01

| Flujo | Test | semilla | p-value | veces observado |
|---|---|---|---|---|
| /dev/urandom | NIST_BlockFreq | 12345 | 0.00225741 | 1 |
| hybrid_prng | NIST_BlockFreq | 65537 | 0.00757203 | 1 |

## Notas de calibracion (medidas en este entorno)

- El test espectral de Marsaglia usa **200000 tuplas** (no 50000): con 50000 tuplas la ocupacion media por celda de la rejilla 16^3 es 12,2 y el p-value fluctua mucho. Medido sobre el MISMO flujo (lcg+msws64, semilla 3): p=0.000432 con 50000 tuplas y p=0.0751181 con 200000. El FAIL de 0.00043 era ruido del test, no un defecto del generador.
- El FAIL de `NIST_BlockFreq` en `hybrid_prng` (semilla 65537, p=0.00757203) es **reproducible**: sale con el mismo valor en corridas independientes. Queda registrado como senal marginal dentro del presupuesto de falsos positivos (1 de 56 frente a 0.56 esperados), no como prueba de defecto.
- `hybrid_prng` combina el XOR de una secuencia middle-square de 4 digitos decimales con `msws64`; la parte middle-square aporta solo ~13,3 bits por palabra, asi que su mitad superior hereda esa limitacion aunque el XOR con msws64 la disperse. Para uso criptografico o de simulacion de Monte Carlo de alta exigencia, usar `msws64` puro.

## Control de calibracion: /dev/urandom (debe aprobar)

| Test | metrica | peor caso | pass |
|---|---|---|---|
| Spectral3D | ratio | 0.994557 | ✅ |
| MarsagliaSpectral3D | p_value | 0.252617 | ✅ |
| BirthdaySpacings | p_value | 0.0354213 | ✅ |
| Permutation | p_value | 0.117718 | ✅ |
| NIST_Frequency | p_value | 0.169251 | ✅ |
| NIST_BlockFreq | p_value | 0.00225741 | ❌ |
| NIST_Runs | p_value | 0.022494 | ✅ |

## Control negativo: estado crudo de randu_plus (debe fallar)

- planos del reticulo: 15 (relacion exacta: True)
- p-value espectral (Marsaglia) del estado CRUDO de randu_plus: 0.0
- p-value espectral de la salida MEZCLADA de randu_plus: 0.6934566560975884
- unmix32(salida) devuelve el estado crudo: True
- veredicto del control negativo: PASS (falló como debe)

Si el control limpio se comportara como un generador roto, o el control negativo no fallara, la tabla de arriba no significaria nada.
