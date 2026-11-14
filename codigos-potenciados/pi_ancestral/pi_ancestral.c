/* pi_ancestral.c - Aproximaciones de pi por poligonos y extrapolacion de Liu Hui
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra pi_ancestral.c pi_ancestral_test.c -lm -o prueba
 * Ejecutar: ./prueba
 *
 * Metodo del perimetro (Arquimedes): el poligono regular de n lados
 * circunscrito a un circulo de radio 1 tiene perimetro n*t con
 * t = tan(pi/n). Partiendo de n = 6 (t = 1/sqrt(3)) y duplicando n en
 * cada paso, el perimetro baja hacia pi desde arriba.
 *
 * La forma ingenua t = (sqrt(1+t^2)-1)/t resta dos numeros casi iguales:
 * cuando t baja de sqrt(eps) la resta se anula y la recursion se rompe.
 * La forma estable t = t/(sqrt(1+t^2)+1) racionaliza y no pierde digitos.
 */

#include "pi_ancestral.h"

#include <math.h>
#include <stdlib.h>

/* Tope de iteraciones: 6*2^60 todavia entra en long y en double */
#define PI_ANCESTRAL_MAX_ITERACIONES 60

static int iteraciones_validas(int iteraciones) {
    if (iteraciones < 0) return 0;
    if (iteraciones > PI_ANCESTRAL_MAX_ITERACIONES) return PI_ANCESTRAL_MAX_ITERACIONES;
    return iteraciones;
}

/* Perimetro con recursion estable (racionalizada) */
double pi_arquimedes_estable(int iteraciones) {
    int k_max = iteraciones_validas(iteraciones);
    double t = 1.0 / sqrt(3.0);
    double n = 6.0;
    int k;
    for (k = 0; k < k_max; ++k) {
        t = t / (sqrt(1.0 + t * t) + 1.0);
        n *= 2.0;
    }
    return n * t;
}

/* Perimetro con recursion ingenua (cancelacion catastrofica) */
double pi_arquimedes_ingenuo(int iteraciones) {
    int k_max = iteraciones_validas(iteraciones);
    double t = 1.0 / sqrt(3.0);
    double n = 6.0;
    int k;
    for (k = 0; k < k_max; ++k) {
        t = (sqrt(1.0 + t * t) - 1.0) / t;
        n *= 2.0;
    }
    return n * t;
}

/* Cotas de Arquimedes sobre semiperimetros
 *
 * Con semiperimetros i (inscrito) y c (circunscrito), al duplicar los lados:
 *   c' = 2*i*c/(i+c)                 (media armonica) -> nuevo circunscrito
 *   i' = i * sqrt(2*c/(i+c))         -> nuevo inscrito
 *
 * Ojo: la media geometrica sqrt(i*c) NO es el nuevo perimetro inscrito
 * (converge a ~3.219546 en vez de a pi); ver la nota del modulo.
 */
void pi_arquimedes_cotas(int iteraciones, double *cota_inferior, double *cota_superior) {
    int k_max = iteraciones_validas(iteraciones);
    double inscrito = 3.0;             /* semiperimetro del hexagono inscrito */
    double circunscrito = 2.0 * sqrt(3.0); /* semiperimetro del hexagono circunscrito */
    int k;
    for (k = 0; k < k_max; ++k) {
        double nuevo_circunscrito = 2.0 * inscrito * circunscrito / (inscrito + circunscrito);
        double nuevo_inscrito = inscrito * sqrt(2.0 * circunscrito / (inscrito + circunscrito));
        inscrito = nuevo_inscrito;
        circunscrito = nuevo_circunscrito;
    }
    if (cota_inferior) *cota_inferior = inscrito;
    if (cota_superior) *cota_superior = circunscrito;
}

/* Extrapolacion de Liu Hui: promedio de perimetros y Richardson con 4^j-1 */
double pi_liu_hui(int iteraciones, int niveles) {
    int k_max = iteraciones_validas(iteraciones);
    int total = k_max + 1;
    double *tabla;
    double t = 1.0 / sqrt(3.0);
    double n = 6.0;
    int j, i, paso;

    if (niveles < 0) niveles = 0;
    if (niveles > k_max) niveles = k_max;

    tabla = (double *)malloc((size_t)total * sizeof(double));
    if (!tabla) return NAN;

    tabla[0] = n * t;
    for (paso = 0; paso < k_max; ++paso) {
        t = t / (sqrt(1.0 + t * t) + 1.0);
        n *= 2.0;
        tabla[paso + 1] = n * t;
    }

    for (j = 1; j <= niveles; ++j) {
        double factor = pow(4.0, (double)j) - 1.0;
        for (i = 0; i + j < total; ++i) {
            tabla[i] = tabla[i + 1] + (tabla[i + 1] - tabla[i]) / factor;
        }
    }

    {
        /* El mejor estimador es el ultimo indice valido: la extrapolacion
         * cancela el termino dominante (error ~ 4^-k) y deja el mas chico */
        double resultado = tabla[total - niveles - 1];
        free(tabla);
        return resultado;
    }
}

/* Tabla historica: lados, perimetros y cotas en cada paso */
int pi_arquimedes_pasos(int iteraciones, pi_arquimedes_paso_t *pasos, int max_pasos) {
    int k_max = iteraciones_validas(iteraciones);
    double t_estable = 1.0 / sqrt(3.0);
    double t_ingenuo = 1.0 / sqrt(3.0);
    double n = 6.0;
    double inscrito = 3.0;
    double circunscrito = 2.0 * sqrt(3.0);
    int k;
    int escritos = 0;

    if (!pasos || max_pasos <= 0) return 0;

    for (k = 0; k <= k_max && escritos < max_pasos; ++k) {
        if (k > 0) {
            t_estable = t_estable / (sqrt(1.0 + t_estable * t_estable) + 1.0);
            t_ingenuo = (sqrt(1.0 + t_ingenuo * t_ingenuo) - 1.0) / t_ingenuo;
            n *= 2.0;
        }
        pasos[escritos].lados = (long)n;
        pasos[escritos].perimetro = n * t_estable;
        pasos[escritos].perimetro_ingenuo = n * t_ingenuo;
        if (k > 0) {
            double nuevo_circunscrito = 2.0 * inscrito * circunscrito / (inscrito + circunscrito);
            double nuevo_inscrito = inscrito * sqrt(2.0 * circunscrito / (inscrito + circunscrito));
            inscrito = nuevo_inscrito;
            circunscrito = nuevo_circunscrito;
        }
        pasos[escritos].cota_inferior = inscrito;
        pasos[escritos].cota_superior = circunscrito;
        escritos++;
    }
    return escritos;
}
