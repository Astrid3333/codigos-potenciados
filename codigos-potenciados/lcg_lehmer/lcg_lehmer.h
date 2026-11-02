/* lcg_lehmer.h - Análisis espectral de LCG de Lehmer (1949)
 *
 * Implementa:
 *   - LCG estándar: x_{n+1} = (a * x_n + c) mod m
 *   - RANDU: a = 65539 = 2^16 + 3, c = 0, m = 2^31
 *   - MINSTD: a = 16807 = 7^5, c = 0, m = 2^31 - 1
 *   - Análisis espectral: distancia entre hiperplanos en k dimensiones
 *   - Test de redes (RANDU: puntos caen en 15 planos en 3D)
 *   - Mejoras: TCR (Teorema Chino del Resto), msws64 híbrido
 *
 * Referencia: Knuth TAOCP Vol 2, cap. 3; Marsaglia 1968 (RANDU)
 */

#ifndef LCG_LEHMER_H
#define LCG_LEHMER_H

#include <stdint.h>
#include <stddef.h>

/* Resultado de análisis espectral */
typedef struct {
    uint64_t a;           /* multiplicador */
    uint64_t c;           /* incremento (0 para Lehmer puro) */
    uint64_t m;           /* módulo */
    uint32_t dim;         /* dimensión k del análisis */
    double min_distance;  /* distancia mínima entre hiperplanos (normalizada) */
    int num_planes;       /* número de planos detectados */
    double spectral_score;/* puntuación 0..1, 1 = ideal */
    int is_bad;           /* 1 si falla criterio de espectral */
} lcg_spectral_result_t;

/* Resultado de prueba de ciclos */
typedef struct {
    uint64_t period;      /* período encontrado */
    int full_period;      /* 1 si alcanza período máximo */
    int zero_sink;        /* 1 si cae en cero */
    int zero_sink_count;  /* cuántos estados caen en cero */
} lcg_period_result_t;

/* Resultado híbrido LCG + msws64 */
typedef struct {
    uint64_t state_lcg;
    uint64_t state_msws;
    uint64_t a;
    uint64_t c;
    uint64_t m;
} hybrid_lcg_msws_t;

/* Inicializa híbrido */
void hybrid_lcg_msws_seed(hybrid_lcg_msws_t *h, uint64_t seed);

/* Genera siguiente valor híbrido */
uint64_t hybrid_lcg_msws_next(hybrid_lcg_msws_t *h);

/* Análisis espectral de un LCG dado (a, c, m, dim) */
lcg_spectral_result_t lcg_spectral_analyze(uint64_t a, uint64_t c, uint64_t m,
                                           uint32_t dim);

/* Prueba de período para LCG */
lcg_period_result_t lcg_period_test(uint64_t a, uint64_t c, uint64_t m,
                                    uint64_t x0, uint64_t max_iter);

/* Test de redes (planes) para m = potencia de 2, dimensión 3 */
int lcg_plane_count_3d(uint64_t a, uint64_t c, uint64_t m);

/* Mejoras: TCR para LCG */
typedef struct {
    uint64_t w1; /* resto mod p1 */
    uint64_t w2; /* resto mod p2 */
    uint64_t p1; /* primo 1 */
    uint64_t p2; /* primo 2 */
    uint64_t M;  /* p1 * p2 */
    uint64_t a;  /* multiplicador LCG */
    uint64_t x;  /* estado LCG */
} tcr_lcg_t;

void tcr_lcg_seed(tcr_lcg_t *t, uint64_t seed, uint64_t p1, uint64_t p2,
                  uint64_t a);
uint64_t tcr_lcg_next(tcr_lcg_t *t);

#endif /* LCG_LEHMER_H */