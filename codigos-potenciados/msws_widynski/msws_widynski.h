#ifndef MSWS_WIDYNSKI_H
#define MSWS_WIDYNSKI_H

#include <stdint.h>

/*
 * msws_widynski.h - Middle Square Weyl Sequence (MSWS) de Widynski (2017)
 *
 * Implementacion propia (formulas del articulo, sin copiar codigo del autor)
 * de la familia MSWS: el estado se eleva al cuadrado, se le suma el
 * incremento acumulado de una secuencia de Weyl y se rota 32 bits dentro del
 * palabra de 64 bits. NO es un xorshift: el xorshift64* de Vigna (desplazamientos
 * 12/25/27 con multiplicador final 0x2545F4914F6CDD1D) es OTRO generador,
 * distinto y con otro origen.
 *
 * Variantes incluidas:
 *   - msws32_widynski_t     : salida de 32 bits (salida oficial del articulo)
 *   - msws64_widynski_t     : salida de 64 bits con DOS carriles (ILP)
 *   - msws64_mono_widynski_t: salida de 64 bits con UN carril
 *
 * Constantes Weyl publicadas en el articulo:
 *   s  = 0xb5ad4eceda1ce2a9   (msws32 y carril 1 de msws64)
 *   s2 = 0x278c5a4d8419fe6b   (carril 2 de msws64)
 * El incremento s debe ser IMPAR (Teorema A: periodo 2^64 de la secuencia de
 * Weyl); por eso la siembra fuerza el bit menos significativo con | 1.
 *
 * Estado (64 bits cada variable): x = estado del generador (valor ya
 * rotado), w = secuencia de Weyl acumulada, s = incremento de Weyl.
 *
 * Nota sobre el nombre de las funciones: los identificadores estan en ingles
 * (`_seed`, `_next`) como en el resto de generadores de 64 bits del repo;
 * la documentacion esta en espanol.
 *
 * Referencia: M. Widynski, "Middle Square Weyl Sequence RNG" (2017),
 * arXiv:1704.00358. La reimplementacion es propia; las secuencias de prueba
 * congeladas en msws_widynski_test.c provienen de la implementacion de
 * referencia del autor y del crate Rust "msws" (ver README.md del modulo).
 */

#define MSWS32_WIDYNSKI_S_PAPER  0xB5AD4ECEDA1CE2A9ULL
#define MSWS64_WIDYNSKI_S1_PAPER 0xB5AD4ECEDA1CE2A9ULL
#define MSWS64_WIDYNSKI_S2_PAPER 0x278C5A4D8419FE6BULL

/* ------------------------------------------------------------------ */
/* Tipos de estado                                                     */
/* ------------------------------------------------------------------ */

/* Variante de 32 bits: estado de 3 palabras de 64 bits. */
typedef struct {
    uint64_t x;  /* estado del generador (ya rotado) */
    uint64_t w;  /* secuencia de Weyl acumulada */
    uint64_t s;  /* incremento de Weyl (impar) */
} msws32_widynski_t;

/* Variante de 64 bits con dos carriles independientes (ILP). */
typedef struct {
    uint64_t x1; /* estado del carril 1 (ya rotado) */
    uint64_t w1; /* Weyl acumulada del carril 1 */
    uint64_t s1; /* incremento de Weyl del carril 1 (impar) */
    uint64_t x2; /* estado del carril 2 (ya rotado) */
    uint64_t w2; /* Weyl acumulada del carril 2 */
    uint64_t s2; /* incremento de Weyl del carril 2 (impar) */
} msws64_widynski_t;

/* Variante de 64 bits con un solo carril (salida de 64 bits completa). */
typedef struct {
    uint64_t x;  /* estado del generador (ya rotado) */
    uint64_t w;  /* secuencia de Weyl acumulada */
    uint64_t s;  /* incremento de Weyl (impar) */
} msws64_mono_widynski_t;

/* ------------------------------------------------------------------ */
/* Utilidad de rotacion                                                */
/* ------------------------------------------------------------------ */

/* Rotacion de 32 bits dentro de una palabra de 64 bits: intercambia las dos
 * mitades. Es la operacion que usa el articulo ("circular shift"). Como
 * rotar por 32 bits es involutivo, rot(rot(v)) == v. */
static inline uint64_t msws_widynski_rot(uint64_t v) {
    return (v >> 32) | (v << 32);
}

/* ------------------------------------------------------------------ */
/* Variante de 32 bits                                                 */
/* ------------------------------------------------------------------ */

/* Inicializa el generador con un incremento de Weyl s.
 * x = 0, w = 0 (estado canonico del articulo) y s forzado a impar.
 * La semilla 0 no deja el generador atrapado en 0 porque el incremento de
 * Weyl es impar y distinto de cero. */
static inline void msws32_widynski_seed(msws32_widynski_t *rng, uint64_t s) {
    rng->x = 0ULL;
    rng->w = 0ULL;
    rng->s = s | 1ULL;
}

/* Inicializacion para flujos paralelos: x = w = s (articulo, seccion
 * "Parallel Usage and Jump Ahead"). Con s distinto en cada flujo y un salto
 * adelante se obtienen flujos disjuntos. */
static inline void msws32_widynski_seed_stream(msws32_widynski_t *rng, uint64_t s) {
    rng->x = s | 1ULL;
    rng->w = s | 1ULL;
    rng->s = s | 1ULL;
}

/* Siguiente valor de 32 bits.
 * Equivale a: x = x*x; x += (w += s); x = rot32(x); se devuelve el estado
 * truncado a 32 bits (la mitad alta del valor previo a la rotacion). */
static inline uint32_t msws32_widynski_next(msws32_widynski_t *rng) {
    uint64_t v;
    rng->x *= rng->x;
    rng->w += rng->s;
    v = rng->x + rng->w;
    rng->x = msws_widynski_rot(v);
    return (uint32_t)(rng->x & 0xFFFFFFFFULL);
}

/* Siguiente double en [0, 1) con 32 bits de precision (articulo). */
static inline double msws32_widynski_next_double(msws32_widynski_t *rng) {
    return (double)msws32_widynski_next(rng) / 4294967296.0;
}

/* Siguiente entero en [0, n) sin sesgo (rechazo por umbral). */
static inline uint32_t msws32_widynski_next_bounded(msws32_widynski_t *rng,
                                                     uint32_t n) {
    uint32_t x;
    uint64_t umbral;
    if (n <= 1U) {
        return 0U;
    }
    umbral = 4294967296ULL % (uint64_t)n;
    do {
        x = msws32_widynski_next(rng);
    } while ((uint64_t)x < umbral);
    return x % n;
}

/* ------------------------------------------------------------------ */
/* Variante de 64 bits con dos carriles                                */
/* ------------------------------------------------------------------ */

/* Inicializa los dos carriles con incrementos de Weyl s1 y s2.
 * x1 = x2 = 0 y w1 = w2 = 0 (estado canonico del articulo). */
static inline void msws64_widynski_seed(msws64_widynski_t *rng,
                                         uint64_t s1, uint64_t s2) {
    rng->x1 = 0ULL;
    rng->w1 = 0ULL;
    rng->s1 = s1 | 1ULL;
    rng->x2 = 0ULL;
    rng->w2 = 0ULL;
    rng->s2 = s2 | 1ULL;
}

/* Inicializacion para flujos paralelos: x1 = w1 = s1 y x2 = w2 = s2. */
static inline void msws64_widynski_seed_stream(msws64_widynski_t *rng,
                                                uint64_t s1, uint64_t s2) {
    rng->x1 = s1 | 1ULL;
    rng->w1 = s1 | 1ULL;
    rng->s1 = s1 | 1ULL;
    rng->x2 = s2 | 1ULL;
    rng->w2 = s2 | 1ULL;
    rng->s2 = s2 | 1ULL;
}

/* Siguiente valor de 64 bits.
 * Carril 1: x1 = x1*x1; w1 += s1; xx = x1 + w1 (SIN rotar); x1 = rot(xx).
 * Carril 2: x2 = x2*x2; w2 += s2; x2 = rot(x2 + w2) (rotado).
 * Salida: xx XOR x2 (asimetria del articulo: solo el carril 1 aporta el
 * valor sin rotar). */
static inline uint64_t msws64_widynski_next(msws64_widynski_t *rng) {
    uint64_t xx;
    rng->x1 *= rng->x1;
    rng->w1 += rng->s1;
    xx = rng->x1 + rng->w1;
    rng->x1 = msws_widynski_rot(xx);
    rng->x2 *= rng->x2;
    rng->w2 += rng->s2;
    rng->x2 = msws_widynski_rot(rng->x2 + rng->w2);
    return xx ^ rng->x2;
}

/* Siguiente double en [0, 1) con 53 bits de precision (articulo). */
static inline double msws64_widynski_next_double(msws64_widynski_t *rng) {
    return (double)(msws64_widynski_next(rng) >> 11) / 9007199254740992.0;
}

/* Siguiente entero en [0, n) sin sesgo (rechazo por umbral). */
static inline uint64_t msws64_widynski_next_bounded(msws64_widynski_t *rng,
                                                     uint64_t n) {
    uint64_t x;
    uint64_t umbral;
    if (n <= 1ULL) {
        return 0ULL;
    }
    umbral = (0ULL - n) % n; /* 2^64 mod n */
    do {
        x = msws64_widynski_next(rng);
    } while (x < umbral);
    return x % n;
}

/* ------------------------------------------------------------------ */
/* Variante de 64 bits con un carril                                   */
/* ------------------------------------------------------------------ */

/* Inicializa con un incremento de Weyl s cualquiera (impar forzado). */
static inline void msws64_mono_widynski_seed_const(msws64_mono_widynski_t *rng,
                                                    uint64_t s) {
    rng->x = 0ULL;
    rng->w = 0ULL;
    rng->s = s | 1ULL;
}

/* Inicializa a partir de una semilla entera de 32/64 bits cualquiera:
 * s = (seed << 1) + 0xb5ad4eceda1ce2a9. Con esta inicializacion la variante
 * mono coincide BIT A BIT con el generador "weyl" de tidwall/weyl-rs. */
static inline void msws64_mono_widynski_seed(msws64_mono_widynski_t *rng,
                                               uint64_t seed) {
    rng->x = 0ULL;
    rng->w = 0ULL;
    rng->s = (seed << 1) + MSWS32_WIDYNSKI_S_PAPER;
}

/* Siguiente valor de 64 bits (estado rotado completo).
 * Su mitad baja coincide con la salida de msws32 con el mismo s. */
static inline uint64_t msws64_mono_widynski_next(msws64_mono_widynski_t *rng) {
    uint64_t v;
    rng->x *= rng->x;
    rng->w += rng->s;
    v = rng->x + rng->w;
    rng->x = msws_widynski_rot(v);
    return rng->x;
}

/* Siguiente double en [0, 1) con 53 bits de precision. */
static inline double msws64_mono_widynski_next_double(msws64_mono_widynski_t *rng) {
    return (double)(msws64_mono_widynski_next(rng) >> 11) / 9007199254740992.0;
}

/* Siguiente entero en [0, n) sin sesgo (rechazo por umbral). */
static inline uint64_t msws64_mono_widynski_next_bounded(msws64_mono_widynski_t *rng,
                                                          uint64_t n) {
    uint64_t x;
    uint64_t umbral;
    if (n <= 1ULL) {
        return 0ULL;
    }
    umbral = (0ULL - n) % n; /* 2^64 mod n */
    do {
        x = msws64_mono_widynski_next(rng);
    } while (x < umbral);
    return x % n;
}

/* ------------------------------------------------------------------ */
/* Salto adelante (jump ahead)                                         */
/* ------------------------------------------------------------------ */
/*
 * Reimplementacion PROPIA, no bit a bit identica a la routine del autor
 * (que solo esta publicada en su cabecera GPLv3).
 *
 * Fundamento (articulo, Teorema A): la secuencia de Weyl cumple
 * w(i) = (i * s) mod 2^64, de modo que avanzar m posiciones de la secuencia
 * equivale a w += m * s (mod 2^64). Ademas el articulo indica que hay que
 * "aleatorizar" el estado x con varias rondas de cuadrado al estilo
 * counter-based squares.
 *
 * Aqui se usan 4 rondas: x = w; y luego x = rot(x*x + w + i) para i = 0..3.
 * El contador i hace que cada ronda use un operando distinto y evita que la
 * ronda 3 repita exactamente el valor de la ronda 1.
 *
 * Consecuencia honesta: las secuencias producidas con salto NO coinciden con
 * las del autor, porque su rutina de aleatorizacion es distinta. Lo que si
 * se garantiza es la propiedad matematica: el acumulador de Weyl queda
 * exactamente m * s por delante, y los flujos obtenidos con saltos distintos
 * son disjuntos y no se solapan.
 */

/* Avanza m posiciones de la secuencia de Weyl y aleatoriza el estado. */
static inline void msws32_widynski_jump_ahead(msws32_widynski_t *rng,
                                                uint64_t m) {
    uint64_t i;
    rng->w += m * rng->s;
    rng->x = rng->w;
    for (i = 0ULL; i < 4ULL; ++i) {
        rng->x = msws_widynski_rot(rng->x * rng->x + rng->w + i);
    }
}

/* Avanza m posiciones en los dos carriles (salto alineado). */
static inline void msws64_widynski_jump_ahead(msws64_widynski_t *rng,
                                                uint64_t m) {
    uint64_t i;
    rng->w1 += m * rng->s1;
    rng->x1 = rng->w1;
    for (i = 0ULL; i < 4ULL; ++i) {
        rng->x1 = msws_widynski_rot(rng->x1 * rng->x1 + rng->w1 + i);
    }
    rng->w2 += m * rng->s2;
    rng->x2 = rng->w2;
    for (i = 0ULL; i < 4ULL; ++i) {
        rng->x2 = msws_widynski_rot(rng->x2 * rng->x2 + rng->w2 + i);
    }
}

/* Avanza m posiciones de la secuencia de Weyl (variante mono). */
static inline void msws64_mono_widynski_jump_ahead(msws64_mono_widynski_t *rng,
                                                     uint64_t m) {
    uint64_t i;
    rng->w += m * rng->s;
    rng->x = rng->w;
    for (i = 0ULL; i < 4ULL; ++i) {
        rng->x = msws_widynski_rot(rng->x * rng->x + rng->w + i);
    }
}

#endif /* MSWS_WIDYNSKI_H */
