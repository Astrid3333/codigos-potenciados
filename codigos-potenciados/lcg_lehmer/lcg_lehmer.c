/* lcg_lehmer.c - Análisis espectral de LCG de Lehmer (1949)
 *
 * Compilar: gcc -std=c11 -O2 lcg_lehmer.c -o lcg_lehmer_test -lm
 * Ejecutar: ./lcg_lehmer_test
 *
 * Incluye:
 *   - Análisis espectral de LCG (distancia entre hiperplanos)
 *   - Test de redes/planos (RANDU: 15 planos en 3D)
 *   - Prueba de período
 *   - Híbrido LCG + msws64
 *   - TCR-LCG (Teorema Chino del Resto)
 *
 * Referencia: Knuth TAOCP Vol 2, cap. 3; Marsaglia 1968 (RANDU)
 */

#include "lcg_lehmer.h"
#include <math.h>

/* Tipos de 128 bits (extensión GCC) para la aritmética modular exacta:
 * (uint64 * uint64) puede exceder 64 bits y desbordaría sin ellos.
 * __extension__ silencia el aviso de -Wpedantic por no ser ISO C. */
__extension__ typedef __int128 lcg_int128_t;
__extension__ typedef unsigned __int128 lcg_uint128_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------- msws64 (Widynski 2017) ---------- */
static uint64_t msws64_next(uint64_t *state) {
    *state *= 6364136223846793005ULL;
    *state ^= *state >> 17;
    *state *= 1442695040888963407ULL;
    *state ^= *state >> 31;
    *state *= 3141592653589793239ULL;
    *state ^= *state >> 27;
    return *state;
}

/* ---------- hybrid_lcg_msws ---------- */
void hybrid_lcg_msws_seed(hybrid_lcg_msws_t *h, uint64_t seed) {
    h->state_lcg = (seed % (UINT64_MAX - 1)) + 1;
    h->state_msws = seed | 1ULL;
    h->a = 65539ULL;   /* RANDU multiplicador */
    h->c = 0ULL;
    h->m = 1ULL << 31; /* RANDU módulo */
}

uint64_t hybrid_lcg_msws_next(hybrid_lcg_msws_t *h) {
    /* LCG paso: x = (a*x + c) mod m */
    h->state_lcg = (h->a * h->state_lcg + h->c) % h->m;
    /* msws64 paso */
    uint64_t mw = msws64_next(&h->state_msws);
    /* Combinar: XOR */
    return h->state_lcg ^ mw;
}

/* ---------- msws64 puro ---------- */
uint64_t msws64_pure_next(uint64_t *state) {
    return msws64_next(state);
}

/* ---------- powmod ---------- */
static uint64_t mod_pow(uint64_t a, uint64_t k, uint64_t m) {
    uint64_t result = 1 % m;
    a = a % m;
    while (k > 0) {
        if (k & 1) result = (uint64_t)((lcg_uint128_t)result * a % m);
        a = (uint64_t)((lcg_uint128_t)a * a % m);
        k >>= 1;
    }
    return result;
}

/* ---------- Análisis espectral ----------
 *
 * Para un LCG x_{n+1} = (a*x_n + c) mod m, los puntos k-dimensionales
 * caen sobre hiperplanos. La distancia mínima entre planos está dada por
 * la menor norma de un vector de números enteros h = (h_1,...,h_k) tal que:
 *
 *   h_1 + h_2*a + h_3*a^2 + ... + h_k*a^(k-1) ≡ 0 (mod m)
 *
 * Para m = potencia de 2, esto se simplifica porque el anillo no es un campo.
 * Usamos búsqueda en rango pequeño para dimensiones pequeñas.
 */

lcg_spectral_result_t lcg_spectral_analyze(uint64_t a, uint64_t c, uint64_t m,
                                           uint32_t dim) {
    lcg_spectral_result_t res = {0};
    res.a = a;
    res.c = c;
    res.m = m;
    res.dim = dim;
    res.num_planes = 0;
    res.is_bad = 1;

    if (dim < 2 || dim > 8) {
        res.spectral_score = 0.0;
        res.min_distance = 0.0;
        return res;
    }

    /* Precalcular potencias de a mod m */
    uint64_t powers[8];
    powers[0] = 1 % m;
    for (uint32_t i = 1; i < dim; i++) {
        powers[i] = (uint64_t)((lcg_uint128_t)powers[i-1] * a % m);
    }

    /* Búsqueda del vector de normales más corto */
    double best_norm = INFINITY;
    int best_count = 0;
    int h_range = 1000; /* rango de búsqueda de vectores h.
     * NOTA: Para m grande (ej. 2^31), el vector más corto puede tener
     * componentes > h_range. Una búsqueda completa requiere reducción
     * de retículo (LLL/Gauss), que no está implementada. Los scores
     * son por tanto una cota inferior de la calidad real. */

    if (dim == 2) {
        /* h1 + h2*a ≡ 0 (mod m) */
        for (int h2 = -h_range; h2 <= h_range; h2++) {
            if (h2 == 0) continue;
            /* h1 = -h2*a mod m */
            lcg_int128_t val = ((lcg_int128_t)(-h2) * (lcg_int128_t)powers[1]) % (lcg_int128_t)m;
            if (val < 0) val += m;
            int64_t h1 = (int64_t)val;
            if (h1 > (int64_t)m / 2) h1 -= (int64_t)m;
            double norm = sqrt((double)(h1*h1 + h2*h2));
            if (norm < best_norm - 1e-9 && norm > 1e-9) {
                best_norm = norm;
                best_count = 1;
            } else if (fabs(norm - best_norm) < 1e-9) {
                best_count++;
            }
        }
    } else if (dim == 3) {
        /* h1 + h2*a + h3*a^2 ≡ 0 (mod m) */
        uint64_t a2 = powers[2];
        for (int h3 = -h_range; h3 <= h_range; h3++) {
            if (h3 == 0) continue;
            for (int h2 = -h_range; h2 <= h_range; h2++) {
                if (h2 == 0) continue;
                lcg_int128_t val = ((lcg_int128_t)h2 * (lcg_int128_t)powers[1] +
                                (lcg_int128_t)h3 * (lcg_int128_t)a2) % (lcg_int128_t)m;
                val = (-val) % (lcg_int128_t)m;
                if (val < 0) val += m;
                int64_t h1 = (int64_t)val;
                if (h1 > (int64_t)m / 2) h1 -= (int64_t)m;
                double norm = sqrt((double)(h1*h1 + h2*h2 + h3*h3));
                if (norm < best_norm - 1e-9 && norm > 1e-9) {
                    best_norm = norm;
                    best_count = 1;
                } else if (fabs(norm - best_norm) < 1e-9) {
                    best_count++;
                }
            }
        }
    } else {
        /* Para dimensiones mayores, aproximación teórica */
        best_norm = pow((double)m, 1.0 / (double)dim);
        best_count = 1;
    }

    /* Distancia mínima entre planos: d = 1 / ||h|| */
    /* Normalizado por m^(1/k): d_norm = d * m^(1/k) */
    /* Un buen LCG tiene muchos planes (d_norm pequeño). Un malo (RANDU) */
    /* tiene pocos planes (d_norm grande). El score se invierte para que */
    /* 1.0 = ideal y valores bajos = malo. */
    if (best_norm < INFINITY && best_norm > 1e-9) {
        res.min_distance = 1.0 / best_norm;
        res.num_planes = best_count;
        double d_norm = res.min_distance * pow((double)m, 1.0 / (double)dim);
        res.spectral_score = 1.0 / d_norm;  /* invertido: alto = bueno */
        if (res.spectral_score > 1.0) res.spectral_score = 1.0;
        /* Umbral: score < 0.3 se considera malo (pocos planes distantes) */
        res.is_bad = (res.spectral_score < 0.3);
    } else {
        res.min_distance = 0.0;
        res.spectral_score = 0.0;
        res.is_bad = 1;
    }

    return res;
}

/* ---------- Prueba de período (método de Floyd) ---------- */
lcg_period_result_t lcg_period_test(uint64_t a, uint64_t c, uint64_t m,
                                    uint64_t x0, uint64_t max_iter) {
    lcg_period_result_t res = {0};
    res.period = 0;
    res.full_period = 0;
    res.zero_sink = 0;
    res.zero_sink_count = 0;

    if (m == 0 || m == 1) return res;

    uint64_t tortoise = x0 % m;
    uint64_t hare = x0 % m;

    /* Fase 1: encontrar punto de encuentro */
    uint64_t steps = 0;
    for (uint64_t i = 0; i < max_iter; i++) {
        tortoise = (a * tortoise + c) % m;
        hare = (a * hare + c) % m;
        hare = (a * hare + c) % m;
        steps++;
        if (tortoise == hare) break;
    }

    /* Fase 2: encontrar longitud del ciclo */
    uint64_t cycle_len = 0;
    hare = tortoise;
    do {
        hare = (a * hare + c) % m;
        cycle_len++;
    } while (hare != tortoise && cycle_len < max_iter);

    res.period = cycle_len;

    /* Determinar full_period */
    if (m == (1ULL << 31)) {
        /* Para m = 2^31, período máximo es 2^29 si a ≡ 3 o 5 (mod 8) */
        res.full_period = (cycle_len == (1ULL << 29));
    } else if (m == ((1ULL << 31) - 1)) {
        res.full_period = (cycle_len == m - 1);
    } else {
        res.full_period = (cycle_len == m - 1 || cycle_len == m);
    }

    /* Contar zero_sink en muestra */
    uint64_t x_test = x0 % m;
    for (uint64_t i = 0; i < (max_iter < 10000 ? max_iter : 10000); i++) {
        if (x_test == 0) {
            res.zero_sink = 1;
            res.zero_sink_count++;
        }
        x_test = (a * x_test + c) % m;
    }

    return res;
}

/* ---------- Test de planos en 3D (RANDU) ----------
 *
 * Para m = 2^p, los puntos (x_n, x_{n+1}, x_{n+2}) caen en planos.
 * El número de planos está dado por el método de Marsaglia.
 * Contamos cuántos valores distintos toma h1 + h2*a + h3*a^2 mod m
 * para h_i en rango pequeño.
 */
int lcg_plane_count_3d(uint64_t a, uint64_t c, uint64_t m) {
    int count = 0;
    int max_h = 50;

    uint64_t a2 = (a * a) % m;

    /* Para x_{n+2} = a*x_{n+1} + c = a*(a*x_n + c) + c = a^2*x_n + a*c + c */
    /* El plano está dado por: h1*x_n + h2*x_{n+1} + h3*x_{n+2} ≡ 0 (mod m) */
    /* Sustituyendo: h1*x + h2*(a*x+c) + h3*(a^2*x+a*c+c) ≡ 0 */
    /* = (h1 + h2*a + h3*a^2)*x + (h2*c + h3*a*c + h3*c) ≡ 0 */
    /* Para que sea plano para todo x: h1 + h2*a + h3*a^2 ≡ 0 */
    /* Y el término constante también debe ser 0: h2*c + h3*a*c + h3*c ≡ 0 */

    for (int h3 = -max_h; h3 <= max_h; h3++) {
        for (int h2 = -max_h; h2 <= max_h; h2++) {
            for (int h1 = -max_h; h1 <= max_h; h1++) {
                if (h1 == 0 && h2 == 0 && h3 == 0) continue;
                /* Verificar h1 + h2*a + h3*a^2 ≡ 0 (mod m) */
                lcg_int128_t val = ((lcg_int128_t)h1 +
                                (lcg_int128_t)h2 * (lcg_int128_t)a +
                                (lcg_int128_t)h3 * (lcg_int128_t)a2) % (lcg_int128_t)m;
                if (val < 0) val += m;
                if (val != 0) continue;

                /* Verificar término constante: h2*c + h3*a*c + h3*c ≡ 0 */
                lcg_int128_t val2 = ((lcg_int128_t)h2 * (lcg_int128_t)c +
                                 (lcg_int128_t)h3 * (lcg_int128_t)a * (lcg_int128_t)c +
                                 (lcg_int128_t)h3 * (lcg_int128_t)c) % (lcg_int128_t)m;
                if (val2 < 0) val2 += m;
                if (val2 != 0) continue;

                /* Plano válido */
                count++;
            }
        }
    }
    return count;
}

/* ---------- TCR-LCG ---------- */
void tcr_lcg_seed(tcr_lcg_t *t, uint64_t seed, uint64_t p1, uint64_t p2,
                  uint64_t a) {
    t->p1 = p1;
    t->p2 = p2;
    t->M = p1 * p2;
    t->a = a;
    t->w1 = seed % p1;
    t->w2 = seed % p2;
    t->x = seed % t->M;
}

uint64_t tcr_lcg_next(tcr_lcg_t *t) {
    /* Avanzar contadores TCR */
    t->w1 = (t->w1 + t->a) % t->p1;
    t->w2 = (t->w2 + t->a) % t->p2;

    /* Reconstruir w via TCR: w ≡ w1 (mod p1), w ≡ w2 (mod p2) */
    /* w = w1 + p1 * k, donde k = ((w2 - w1) * inv(p1 mod p2)) mod p2 */
    uint64_t inv_p1 = mod_pow(t->p1 % t->p2, t->p2 - 2, t->p2); /* Fermat: p2 primo */
    uint64_t k = (((t->w2 - t->w1) % t->p2 + t->p2) % t->p2) * inv_p1 % t->p2;
    uint64_t w = (t->w1 + t->p1 * k) % t->M;

    /* Avanzar LCG: x_{n+1} = a * x_n mod M */
    t->x = (t->a * t->x) % t->M;

    /* Combinar: XOR */
    return t->x ^ w;
}