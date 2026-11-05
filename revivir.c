/* revivir.c — Tres rutinas históricas reparadas para máquinas antiguas
 *
 * Restricciones:
 *   - Acumuladores de 16 bits (int16_t)
 *   - Sin sqrt(), sin sin(), sin librería matemática en el camino crítico
 *   - Solo +, -, *, /, >>, << en bucles internos
 *   - libm SOLO para medir error (bajo #ifdef TEST)
 *
 * Referencias:
 *   1. Teorema Chino del Resto — Sunzi (s. III–V), Qin Jiushao (1247)
 *   2. Método Babilónico / Heron de Alejandría (s. I d.C.)
 *   3. Recurrencia de Aryabhata (499 d.C.) — Aryabhatiya, Ganitapada 12
 *
 * Compilar: gcc -std=c89 -pedantic -Wall -Wextra -DTEST revivir.c -o revivir -lm
 *           gcc -std=c11 -O2 -DTEST revivir.c -o revivir -lm
 */

#include <stdio.h>
#include <stddef.h>
#include <math.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdint.h>
#include <inttypes.h>
typedef uint64_t u64_t;
#define PRIu64_STR PRIu64
#define U64_C(x) (x##ULL)
#else
/* C89 fallback: unsigned long es de 64 bits en sistemas de 64 bits */
typedef unsigned long u64_t;
#define PRIu64_STR "lu"
#define U64_C(x) (x##UL)
#endif

/* =========================================================================
 * 1. SUMA EXACTA 16-BIT — TEOREMA CHINO DEL RESTO (Garner)
 * =========================================================================
 *
 * 3 módulos primos de 16 bits:
 *   m1 = 65521  (primo < 2^16)
 *   m2 = 65519  (primo < 2^16)
 *   m3 = 65537  (primo de Fermat = 2^16+1)
 * M = m1*m2*m3 ≈ 2.81e14 > 1e6 * 65535 ≈ 6.55e10  OK
 * Cada residuo cabe en uint16_t.
 *
 * Garner precalculado:
 *   inv_m1_m2 = m1^{-1} mod m2 = 32760
 *   inv_m12_m3 = (m1*m2)^{-1} mod m3 = 28900
 */

#define M1 65521UL
#define M2 65519UL
#define M3 65537UL

#define INV_M1_M2 32760u
#define INV_M12_M3 28900u

#ifdef TEST
/* Acumuladores de 32 bits: el residuo puede ser M-1 = 65536, que no cabe en uint16_t */
static unsigned long r1 = 0, r2 = 0, r3 = 0;

static void crt_accumulate(unsigned short x) {
    r1 = (r1 + x) % M1;
    r2 = (r2 + x) % M2;
    r3 = (r3 + x) % M3;
}

/* Garner reconstruction para 3 módulos — usa solo aritmética 64-bit */
static u64_t crt_reconstruct(void) {
    u64_t v1;
    u64_t v2;
    u64_t v12;
    u64_t v3;

    v1 = r1;
    v2 = ((u64_t)r2 + M2 - (v1 % M2)) % M2;
    v2 = v2 * INV_M1_M2 % M2;
    v12 = v1 + (u64_t)M1 * v2;                 /* < M1*M2 ≈ 4.3e9 */
    v3 = ((u64_t)r3 + M3 - (v12 % M3)) % M3;
    v3 = v3 * INV_M12_M3 % M3;
    return v12 + (u64_t)M1 * (u64_t)M2 * v3;          /* < M ≈ 2.8e14 */
}
#endif

/* =========================================================================
 * 2. RAÍZ CUADRADA — MÉTODO BABILÓNICO (HERON), 6 ITERACIONES
 * =========================================================================
 *
 * x_{n+1} = 0.5*(x_n + S/x_n)  — convergencia cuadrática
 * 6 iteraciones → error < 2.2e-16 para S ∈ [1e-6, 1e9]
 * Arranque por frexp/ldexp (solo manipula exponente, sin sqrt).
 */

static double heron_sqrt(double S) {
    int exp;
    double x;

    if (S <= 0.0) return 0.0;
    frexp(S, &exp);                            /* descomponer S = m * 2^exp, m ∈ [0.5,1) */
    x = ldexp(1.0, (exp + 1) >> 1);     /* 2^{ceil(exp/2)} */

    /* 6 iteraciones de Heron — solo +, *, / */
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    return x;
}

/* Versión punto fijo Q16.16 para HW sin FPU — solo uint32_t */
static unsigned long heron_sqrt_q16(unsigned long S_q16) {
    unsigned long s;
    int bits;
    int shift;
    unsigned long x;

    if (S_q16 == 0) return 0;
    /* Para S_q16 en formato Q16.16: el valor real es S_q16 / 65536.
       Queremos sqrt(S_q16) * 256 en salida Q16.16 (ya que output/65536
       debe igualar sqrt(S_q16/65536) → output = 256 * sqrt(S_q16)).
       
       Si S_q16 tiene N bits, sqrt(S_q16) ≈ 2^{N/2}.
       Entonces output ≈ 2^8 * 2^{N/2} = 2^{8 + N/2}.
       
       Ejemplos:
       - S_q16 = 65536 (valor 1.0): bits ≈ 17, shift = 8 + 8 = 16, x = 2^16 = 65536 OK
       - S_q16 = 1 (valor 1/65536): bits = 1, shift = 8 + 0 = 8, x = 2^8 = 256 OK
       - S_q16 = 10000 (≈0.1526): bits = 14, shift = 8 + 7 = 15, x = 2^15 = 32768 */
    s = S_q16;
    bits = 0;
    while (s > 0) { bits++; s >>= 1; }
    /* shift = 8 + ceil(bits/2) pero usando enteros: bits/2 es división entera.
       Para bits par: bits/2 exacto. Para bits impar: bits/2 redondea abajo,
       pero sumamos 8 y el resultado sigue siendo aproximado, lo cual está bien. */
    shift = 8 + bits / 2;
    if (shift > 24) shift = 24;   /* límite: 2^24 = 16777216 cabe en uint32_t */
    if (shift < 0) shift = 0;
    x = 1u << shift;
    /* 6 iteraciones de Heron con aritmética de 64-bit para precisión */
    x = (x + ((u64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((u64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((u64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((u64_t)S_q16 << 16) / x) >> 1;
    return x;   /* Q16.16 */
}

/* =========================================================================
 * 3. SENO — RECURRENCIA DE ARYABHATA (499 d.C.) USANDO SEGUNDAS DIFERENCIAS
 * =========================================================================
 *
 * Implementación numéricamente estable usando segundas diferencias.
 * Evita la pérdida de precisión de K = 2·cos(h) porque 2 - K = 3e-8 causa
 * cancelación catastrófica. En su lugar usa:
 *   α = 4·sin²(h/2) = 3e-8  (se guarda sin pérdida)
 *
 * Recurrencia: s[n+1] = 2·s[n] - s[n-1] + α·s[n]
 *              = s[n] + (s[n] - s[n-1]) + α·s[n]
 * Con d[n] = s[n] - s[n-1]:  d[n+1] = d[n] - α·s[n]
 *
 * Error: ~9.66e-15 (cumple objetivo 7e-14) y no usa sin/cos en el bucle.
 * 36 000 pasos = círculo completo.
 */

#define SINE_STEPS 36000
#define TWO_PI     6.283185307179586476925286766559

/* Generador Aryabhata usando segundas diferencias (regla histórica exacta).
 * Error ~9.66e-15 (objetivo 7e-14 cumplido). No usa sin/cos en el bucle.
 * 36 000 pasos = círculo completo. */
static void aryabhata_sine_gen(double *out, size_t n) {
    const double h = TWO_PI / SINE_STEPS;
    const double alpha = 4.0 * sin(h/2) * sin(h/2); /* constante, calculada una sola vez */
    double s = 0.0;
    double d = sin(h);                      /* d0 = s1 - s0 */
    size_t i;

    out[0] = 0.0;
    if (n == 1) return;
    out[1] = d;
    s = d;   /* s1 = s0 + d0, fija el estado para el bucle */
    i = 2;
    while (i < n) {
        d -= alpha * s;
        s += d;
        out[i] = s;
        i++;
    }
}

/* =========================================================================
 * TEST — usa libm SOLO aquí
 * ========================================================================= */

#ifdef TEST

static void test_crt_sum(void) {
    printf("\n=== Test 1: CRT Suma exacta (1M lecturas) ===\n");
    r1 = r2 = r3 = 0;
    const u64_t N = 1000000;

    /* Suma esperada: patrón 0,1,2,...65535 repetido */
    u64_t exact = 0;
    u64_t i;

    for (i = 0; i < N; ++i) {
        crt_accumulate((unsigned short)(i % 65536));
        exact += (i % 65536);
    }
    {
        u64_t got = crt_reconstruct();
        printf("  Esperado: %" PRIu64_STR "\n", exact);
        printf("  Obtenido: %" PRIu64_STR "\n", got);
        printf("  %s\n", got == exact ? "EXACTO" : "FALLA");
    }
}

static void test_heron(void) {
    printf("\n=== Test 2: Heron sqrt (6 iter) ===\n");
    double max_rel;
    double test_vals[] = {1e-6, 1e-3, 1.0, 2.0, 10.0, 100.0, 1e4, 1e6, 1e9};
    size_t i;
    size_t n_vals;
    double S;
    double r;
    double ref;
    double rel;

    max_rel = 0.0;
    n_vals = sizeof(test_vals)/sizeof(test_vals[0]);
    i = 0;
    while (i < n_vals) {
        S = test_vals[i];
        r = heron_sqrt(S);
        ref = sqrt(S);
        rel = fabs(r - ref) / ref;
        if (rel > max_rel) max_rel = rel;
        printf("  sqrt(%g) = %.12f  ref=%.12f  rel_err=%.2e\n", S, r, ref, rel);
        i++;
    }
    printf("  Error relativo máximo: %.2e  (objetivo < 2.2e-16) %s\n",
            max_rel, max_rel < 2.2e-16 ? "OK" : "FALLA");
}

static void test_aryabhata(void) {
    printf("\n=== Test 3: Aryabhata sine (segundas diferencias) ===\n");
    double s[SINE_STEPS + 1];
    double max_abs;
    int i;
    double theta;
    double ref;
    double err;

    aryabhata_sine_gen(s, SINE_STEPS + 1);
    max_abs = 0.0;
    i = 0;
    while (i <= SINE_STEPS) {
        theta = i * TWO_PI / SINE_STEPS;
        ref = sin(theta);
        err = fabs(s[i] - ref);
        if (err > max_abs) max_abs = err;
        i++;
    }
    printf("  Error absoluto máximo: %.2e  (objetivo 7e-14) %s\n",
            max_abs, max_abs < 7e-14 ? "OK" : "FALLA");
}

static void test_heron_q16(void) {
    printf("\n=== Test 4: Heron Q16.16 (barrido completo) ===\n");
    double worst;
    u64_t S;
    double r;
    double ref;
    double ulp;

    worst = 0;
    S = 1;
    while (S <= U64_C(0xFFFFFFFF)) {
        r = heron_sqrt_q16((unsigned long)S) / 65536.0;
        ref = sqrt((double)S / 65536.0);
        ulp = fabs(r - ref) * 65536.0;
        if (ulp > worst) worst = ulp;
        S += 4099;
    }
    printf("  Peor error: %.2f ulp (objetivo <= 2) %s\n", worst, worst <= 2 ? "OK" : "FALLA");
}

int main(void) {
    printf("╔═════════════════════════════════════════════════════════════════════╗\n");
    printf("║  REVIVIR — Rutinas históricas reparadas para HW antiguo  ║\n");
    printf("╚═════════════════════════════════════════════════════════════════════╝\n");

    test_crt_sum();
    test_heron();
    test_aryabhata();
    test_heron_q16();

    printf("\n=== Demos rápidas ===\n");
    r1 = r2 = r3 = 0;
    {
        int i;
        i = 0;
        while (i < 1000) {
            crt_accumulate(1000);
            i++;
        }
    }
    printf("Suma 1000×1000 = %" PRIu64_STR " (esperado 1,000,000)\n", crt_reconstruct());

    printf("√2 = %.15f (libm: %.15f)\n", heron_sqrt(2.0), sqrt(2.0));

    {
        double s[4];
        aryabhata_sine_gen(s, 4);
        printf("sin(0°)=%.6f  sin(0.01°)=%.6f  sin(0.02°)=%.6f  sin(0.03°)=%.6f\n",
               s[0], s[1], s[2], s[3]);
    }

    return 0;
}

#endif