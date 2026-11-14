/* middle_square.c - Generador middle square (von Neumann) de 4 digitos
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra middle_square.c middle_square_test.c -lm -o prueba
 */

#include "middle_square.h"

/* x' = los 4 digitos centrales de x*x (escrito con 8 digitos) */
static uint32_t middle_square_paso(uint32_t estado) {
    uint32_t cuadrado = estado * estado; /* cabe en 32 bits: 9999^2 < 10^8 */
    return (cuadrado / 100U) % MIDDLE_SQUARE_TAMANO;
}

void middle_square_semilla(middle_square_t *rng, uint32_t semilla) {
    if (!rng) {
        return;
    }
    rng->estado = semilla % MIDDLE_SQUARE_TAMANO;
}

uint32_t middle_square_siguiente(middle_square_t *rng) {
    if (!rng) {
        return 0U;
    }
    rng->estado = middle_square_paso(rng->estado);
    return rng->estado;
}

double middle_square_siguiente_double(middle_square_t *rng) {
    return (double)middle_square_siguiente(rng) / (double)MIDDLE_SQUARE_TAMANO;
}

uint32_t middle_square_siguiente_acotado(middle_square_t *rng, uint32_t n) {
    uint32_t x;
    uint32_t limite;
    if (!rng || n == 0U) {
        return 0U;
    }
    limite = MIDDLE_SQUARE_TAMANO - (MIDDLE_SQUARE_TAMANO % n);
    do {
        x = middle_square_siguiente(rng);
    } while (x >= limite);
    return x % n;
}

unsigned int middle_square_periodo(middle_square_t *rng, unsigned int max_pasos) {
    uint32_t inicial;
    unsigned int pasos;
    if (!rng || max_pasos == 0U) {
        return 0U;
    }
    inicial = rng->estado;
    for (pasos = 1U; pasos <= max_pasos; ++pasos) {
        if (middle_square_siguiente(rng) == inicial) {
            return pasos;
        }
    }
    return 0U;
}

unsigned int middle_square_periodo_ciclo(uint32_t semilla, unsigned int max_pasos) {
    unsigned char visto[MIDDLE_SQUARE_TAMANO];
    uint32_t estado = semilla % MIDDLE_SQUARE_TAMANO;
    unsigned int pasos = 0U;
    size_t i;
    for (i = 0U; i < MIDDLE_SQUARE_TAMANO; ++i) {
        visto[i] = 0U;
    }
    while (pasos < max_pasos && !visto[estado]) {
        visto[estado] = 1U;
        estado = middle_square_paso(estado);
        pasos++;
    }
    if (!visto[estado]) {
        return 0U; /* no cerro el ciclo dentro del limite */
    }
    /* estado ya estaba visitado: el ciclo empieza en la posicion guardada */
    {
        unsigned int inicio = 0U;
        uint32_t recorre = semilla % MIDDLE_SQUARE_TAMANO;
        while (recorre != estado) {
            recorre = middle_square_paso(recorre);
            inicio++;
        }
        return pasos - inicio;
    }
}

size_t middle_square_secuencia(uint32_t semilla, uint32_t *destino, size_t capacidad) {
    uint32_t estado = semilla % MIDDLE_SQUARE_TAMANO;
    size_t escritos = 0U;
    if (!destino) {
        return 0U;
    }
    while (escritos < capacidad) {
        size_t i;
        int repetido = 0;
        for (i = 0U; i < escritos; ++i) {
            if (destino[i] == estado) {
                repetido = 1;
                break;
            }
        }
        if (repetido) {
            break; /* el ciclo cerro: no se escribe el estado repetido */
        }
        destino[escritos] = estado;
        escritos++;
        estado = middle_square_paso(estado);
    }
    return escritos;
}
