/* abacus_vedic - Suanpan (abaco chino) y multiplicacion vedic (urdhva)
 *
 * El estado de un abaco se guarda por poste, de la UNIDAD hacia la IZQUIERDA
 * (indice 0 = unidades), que es el orden en que viajan los acarreos.
 *
 * Cada poste tiene dos cuentas de CIELO (valor 5 cada una) y cinco de TIERRA
 * (valor 1 cada una). Para el calculo se usa la forma canonica sin
 * redundancia: cielo = 0/1 y tierra = 0..4, de modo que el digito del poste
 * es 5*cielo + tierra (0..9). Es la misma convencion que usan las
 * herramientas Octave `suanpan_add` y `vedic_multiply` del ecosistema MCP.
 */

#ifndef ABACUS_VEDIC_H
#define ABACUS_VEDIC_H

#include <stddef.h>

/* 20 postes alcanza para todo unsigned long long (20 digitos decimales) */
#define SUANPAN_MAX_DIGITOS 20

#define ABACUS_OK             0
#define ABACUS_ERROR_ARGUMENTO (-1)
#define ABACUS_ERROR_DESBORDE  (-2)
#define ABACUS_ERROR_CAPACIDAD (-3)

typedef struct {
    int digitos;                          /* postes usados (indice 0 = unidades) */
    int cielo[SUANPAN_MAX_DIGITOS];       /* 1 = cuenta de cielo presente (vale 5) */
    int tierra[SUANPAN_MAX_DIGITOS];      /* 0..4 cuentas de tierra (valor 1 cada una) */
} suanpan_estado_t;

/* --- representacion ----------------------------------------------------- */

/* Codifica un entero: cada digito decimal d -> cielo = (d >= 5), tierra = d % 5 */
int suanpan_desde_decimal(suanpan_estado_t *estado, unsigned long long valor);

/* Decodifica el abaco a entero (invariante de la codificacion) */
unsigned long long suanpan_a_decimal(const suanpan_estado_t *estado);

/* Convierte a las cadenas de la herramienta Octave: indice 0 = digito MAS
 * SIGNIFICATIVO, con la misma convencion cielo/tierra. Devuelve ABACUS_OK o
 * ABACUS_ERROR_CAPACIDAD si el buffer es corto. */
int suanpan_a_cadenas(const suanpan_estado_t *estado, int *cielo, int *tierra,
                      size_t capacidad);

/* Inverso de suanpan_a_cadenas: las cadenas van de mas a menos significativo */
int suanpan_desde_cadenas(suanpan_estado_t *estado, const int *cielo,
                          const int *tierra, size_t n);

/* --- aritmetica sobre las cuentas --------------------------------------- */

/* Suma con acarreo de poste en poste */
int suanpan_suma(suanpan_estado_t *resultado, const suanpan_estado_t *a,
                 const suanpan_estado_t *b);

/* Resta con préstamo de poste en poste; exige a >= b */
int suanpan_resta(suanpan_estado_t *resultado, const suanpan_estado_t *a,
                  const suanpan_estado_t *b);

/* Multiplica el abaco por un digito decimal (0..9) */
int suanpan_multiplica_digito(suanpan_estado_t *resultado,
                              const suanpan_estado_t *a, int digito);

/* --- multiplicacion vedic (urdhva) --------------------------------------- */

/* Multiplicacion vedic: producto parcial por cada digito del multiplicador,
 * desplazado en potencias de diez y sumado. Sin multiplicacion "larga". */
int vedic_multiplicar(suanpan_estado_t *resultado, const suanpan_estado_t *a,
                      const suanpan_estado_t *b);

/* Productos cruzados digito a digito (convolucion), indice 0 = unidades.
 * Misma cantidad que devuelve `cross_products` en la herramienta Octave. */
int vedic_cruces(const suanpan_estado_t *a, const suanpan_estado_t *b,
                 unsigned long long cruces[], size_t capacidad);

/* Tabla de multiplicar vedic: filas[d-1] = multiplicando * d para d = 1..9 */
int vedic_tabla(unsigned long long multiplicando, unsigned long long filas[9]);

#endif /* ABACUS_VEDIC_H */
