/* middle_square.h - Generador de von Neumann (middle square) de 4 digitos
 *
 * Estado: 4 digitos decimales (0..9999).
 * Paso: se eleva el estado al cuadrado y se conservan los 4 digitos
 * centrales (los postes 2..5 del cuadrado escrito con 8 digitos, con ceros a
 * la izquierda), es decir  x' = (x*x / 100) mod 10000.
 *
 * Es el mismo criterio que usa la funcion estatica `ms_next_4` de
 * ../mult_xorshift64/mult_xorshift64.c (que no se exporta), por lo que este modulo es
 * intercambiable con ella.
 */

#ifndef MIDDLE_SQUARE_H
#define MIDDLE_SQUARE_H

#include <stddef.h>
#include <stdint.h>

#define MIDDLE_SQUARE_MAX_VALOR 9999U
#define MIDDLE_SQUARE_TAMANO    10000U

typedef struct {
    uint32_t estado;
} middle_square_t;

/* Fija la semilla (se recorta a 4 digitos). La semilla 0 es un sumidero:
 * el estado se queda en 0 para siempre */
void middle_square_semilla(middle_square_t *rng, uint32_t semilla);

/* Avanza el generador y devuelve el nuevo estado */
uint32_t middle_square_siguiente(middle_square_t *rng);

/* Avanza y devuelve un double en [0, 1) */
double middle_square_siguiente_double(middle_square_t *rng);

/* Avanza y devuelve un entero en [0, n) por rechazo (sin sesgo) */
uint32_t middle_square_siguiente_acotado(middle_square_t *rng, uint32_t n);

/* Periodo del ciclo que contiene al estado actual: pasos hasta volver al
 * estado inicial. Devuelve 0 si dentro de max_pasos no se cierra el ciclo */
unsigned int middle_square_periodo(middle_square_t *rng, unsigned int max_pasos);

/* Longitud del ciclo eventual que se alcanza desde una semilla cualquiera
 * (mide el transitorio y luego el ciclo). 0 si no cicla en max_pasos */
unsigned int middle_square_periodo_ciclo(uint32_t semilla, unsigned int max_pasos);

/* Escribe la secuencia de estados desde la semilla (inclusive) hasta que se
 * repite o se agota la capacidad. Devuelve cuantos estados escribio */
size_t middle_square_secuencia(uint32_t semilla, uint32_t *destino, size_t capacidad);

#endif /* MIDDLE_SQUARE_H */
