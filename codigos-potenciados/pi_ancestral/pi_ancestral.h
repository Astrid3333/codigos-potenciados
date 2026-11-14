/* pi_ancestral.h - Aproximaciones de pi por poligonos y extrapolacion de Liu Hui
 *
 * Reune las dos vias historicas para acortar el perimetro de un poligono
 * regular hasta pi:
 *
 *   - Perimetro n*t con t = tan(pi/n): el metodo atribuido a Arquimedes.
 *     n = 6*2^k. La recursion "ingenua" t = (sqrt(1+t^2)-1)/t sufre
 *     cancelacion catastrofica; la "estable" t = t/(sqrt(1+t^2)+1) no.
 *
 *   - Cotas por medias: la media armonica acota pi por debajo y la media
 *     geometrica por arriba, partiendo del hexagono inscrito (3) y del
 *     hexagono circunscrito (2*sqrt(3)).
 *
 *   - Extrapolacion de Liu Hui: promedio de los perimetros y refinamiento
 *     de Richardson con factor 4^j-1 sobre la secuencia de perimetros.
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra pi_ancestral.c pi_ancestral_test.c -lm -o prueba
 */

#ifndef PI_ANCESTRAL_H
#define PI_ANCESTRAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Numero maximo de pasos que pi_arquimedes_pasos puede registrar */
#define PI_ANCESTRAL_MAX_PASOS 64

/* Perimetro del poligono circunscrito con 6*2^iteraciones lados */
double pi_arquimedes_estable(int iteraciones);

/* Perimetro con la recursion que cancela (colapsa a 0 o NaN) */
double pi_arquimedes_ingenuo(int iteraciones);

/* Cotas de Arquimedes: inferior <= pi <= superior */
void pi_arquimedes_cotas(int iteraciones, double *cota_inferior, double *cota_superior);

/* Extrapolacion de Liu Hui (Richardson con 4^j-1) sobre los perimetros */
double pi_liu_hui(int iteraciones, int niveles);

/* Un paso de la tabla historica de Arquimedes */
typedef struct {
    long   lados;                /* 6 * 2^k */
    double perimetro;            /* n*t con recursion estable */
    double perimetro_ingenuo;    /* n*t con recursion ingenua */
    double cota_inferior;        /* media armonica */
    double cota_superior;        /* media geometrica */
} pi_arquimedes_paso_t;

/* Registra hasta max_pasos pasos de la tabla; devuelve cuantos escribio */
int pi_arquimedes_pasos(int iteraciones, pi_arquimedes_paso_t *pasos, int max_pasos);

#ifdef __cplusplus
}
#endif

#endif /* PI_ANCESTRAL_H */
