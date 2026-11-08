/* matematica_ancestral.h - Algoritmos matemáticos de tradiciones ancestrales
 *
 * Implementa algoritmos matemáticos de distintas culturas del mundo,
 * muchos de los cuales preceden o son independientes de las formulaciones
 * europeas modernas. Cada implementación incluye notas sobre su origen
 * histórico y contexto cultural.
 *
 * Funciones incluidas:
 *   - multiplicacion_egipcia()   : Egipto antiguo (duplicación y adición)
 *   - fracciones_egipcias()      : Egipto antiguo (fracciones unitarias)
 *   - raiz_cuadrada_babilonia()  : Mesopotamia antigua (método de Heron)
 *   - seno_aryabhata()           : India antigua (recurrencia de segundas diferencias)
 *   - teorema_chino_resto()      : China antigua (Sunzi, Qin Jiushao)
 *   - sistema_vigesimal_maya()   : Mesoamérica (base 20, concepto de cero)
 *   - eliminacion_china()        : China antigua (método de los nueve capítulos)
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra matematica_ancestral.c -lm -o prueba
 * Ejecutar: ./prueba
 */

#ifndef MATEMATICA_ANCESTRAL_H
#define MATEMATICA_ANCESTRAL_H

#include <stdint.h>
#include <stddef.h>

/* ---------- 1. Multiplicación egipcia (duplicación y adición) ---------- */

/* Multiplica dos enteros usando el método egipcio de duplicación:
 * x * y = sumar x duplicado n veces según los bits de n.
 * Complejidad: O(log n) sumas.
 */
int64_t multiplicacion_egipcia(int64_t a, int64_t b);

/* ---------- 2. Fracciones egipcias (algoritmo golardo) ---------- */

/* Descompone una fracción p/q en suma de fracciones unitarias (1/n)
 * usando el algoritmo golardo (resto menor).
 * Devuelve el número de términos y llena denominadores[].
 */
int fracciones_egipcias(int numerador, int denominador,
                        int64_t denominadores[], int max_terminos);

/* ---------- 3. Raíz cuadrada babilónica (método de Heron) ---------- */

/* Calcula sqrt(x) usando el método babilónico (también llamado
 * método de Heron), documentado en tablillas babilónicas del
 * segundo milenio a.C.
 */
double raiz_cuadrada_babilonia(double x, int iteraciones);

/* ---------- 4. Seno de Aryabhata (India, 499 d.C.) ---------- */

/* Calcula sin(theta) usando la recurrencia de segundas diferencias
 * de Aryabhata, descrita en el Aryabhatiya (499 d.C.).
 * theta en radianes. */
double seno_aryabhata(double theta, int pasos);

/* ---------- 5. Teorema Chino del Resto (China antigua) ---------- */

/* Resuelve el sistema de congruencias:
 *   x ≡ a1 (mod m1)
 *   x ≡ a2 (mod m2)
 *   ...
 * Requiere que los módulos sean coprimos entre sí.
 * Devuelve la solución mínima no negativa.
 * El parámetro M_result (si no es NULL) recibe el producto de los módulos.
 */
int64_t teorema_chino_resto(const int64_t residuos[],
                            const int64_t modulos[],
                            int n,
                            int64_t *M_result);

/* ---------- 6. Sistema vigesimal maya ------------------------------ */

/* Convierte un entero decimal a representación maya vigesimal (base 20).
 * Los dígitos se almacenan en digitos[] (menos significativo primero).
 * Devuelve el número de dígitos.
 */
int sistema_vigesimal_maya(uint64_t valor, int digitos[], int max_digitos);

/* ---------- 7. Eliminación china (método de los nueve capítulos) ---------- */

/* Resuelve un sistema Ax = b de tamaño n x n usando eliminación
 * gaussiana estilo chino (de los Nueve Capítulos, siglo I d.C.).
 * Devuelve 0 si tiene solución única, -1 si es singular.
 * La solución se almacena en x[].
 */
int eliminacion_china(double **A, double *b, double *x, int n);

/* ---------- Utilidades -------------------------------- */

/* Inverso modular de a módulo m (mcd(a, m) debe ser 1).
 * Devuelve el inverso en [0, m-1], o -1 si a no es invertible.
 */
int64_t inverso_modular(int64_t a, int64_t m);

/* Convierte un valor en Q15 (punto fijo) a double */
double q15_a_double(int32_t q15);

/* Convierte un double a Q15 */
int32_t double_a_q15(double valor);

#endif /* MATEMATICA_ANCESTRAL_H */