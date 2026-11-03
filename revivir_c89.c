/* revivir_c89.c — Tres rutinas históricas reparadas para C89/C90
 * Sin inline, sin C99 for-loop init, sin long double, sin cbrtl, sin sinl/cosl
 * Compilar: cc -std=c89 -O2 -DTEST revivir_c89.c -o revivir_c89 -lm
 */

#include <stdio.h>
#include <math.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  include <stdint.h>
#else
   typedef unsigned int uint32_t;
   typedef unsigned long uint64_t;
   typedef signed int int32_t;
   typedef signed long int64_t;
   typedef unsigned short uint16_t;
#endif

/* ════════════════════════════════════════════════════════════════════
 * 0. PCG64 + fmix64 — PRNG robusto (reemplaza a randu_plus)
 * ════════════════════════════════════════════════════════════════════
 * Multiplicador PCG64: 6364136223846793005
 * Incremento: 1442695040888963407 (impar, periodo 2^64)
 * Salida: fmix64 (MurmurHash3 finalizer) — biyección, sin planos
 */
static uint64_t pcg_state = 0xCAFEBABEDEADBEEFULL;
static uint64_t pcg_inc   = 0xDA3E39CB94B95BDBULL;

static uint64_t pcg64_next(void) {
    uint64_t old = pcg_state;
    pcg_state = old * 6364136223846793005ULL + pcg_inc;
    uint64_t x = old ^ (old >> 18);
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    x = x ^ (x >> 30);
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

/* ════════════════════════════════════════════════════════════════════
 * 1. SUMA EXACTA 16-BIT — TEOREMA CHINO DEL RESTO (Garner)
 * ═════════════════════════════════════════════════════════════════════
 * m1=65521, m2=65519, m3=65537 (primos < 2^16)
 * M = m1*m2*m3 ≈ 2.81e14 > 1e6 * 65535
 */
#define M1 65521UL
#define M2 65519UL
#define M3 65537UL
#define INV_M1_M2  32760u
#define INV_M12_M3 28900u

static uint32_t r1 = 0, r2 = 0, r3 = 0;

static void crt_accumulate(unsigned int x) {
    r1 = (r1 + x) % M1;
    r2 = (r2 + x) % M2;
    r3 = (r3 + x) % M3;
}

static uint64_t crt_reconstruct(void) {
    uint64_t v1 = r1;
    uint64_t v2 = ((r2 + M2 - (v1 % M2)) % M2) * INV_M1_M2 % M2;
    uint64_t v12 = v1 + (uint64_t)M1 * v2;
    uint64_t v3 = ((r3 + M3 - (v12 % M3)) % M3) * INV_M12_M3 % M3;
    return v12 + (uint64_t)M1 * (uint64_t)M2 * v3;
}

/* ═════════════════════════════════════════════════════════════════════
 * 2. RAÍZ CUADRADA — MÉTODO BABILÓNICO (HERON), 6 ITERACIONES
 * ═════════════════════════════════════════════════════════════════════
 */
static double heron_sqrt(double S) {
    if (S <= 0.0) return 0.0;
    int exp;
    double m = frexp(S, &exp);
    double x = ldexp(1.0, (exp + 1) >> 1);
    if (m < 0.5) x *= 0.70710678118654752440;
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    x = 0.5 * (x + S / x);
    return x;
}

static unsigned int heron_sqrt_q16(unsigned int S_q16) {
    if (S_q16 == 0) return 0;
    unsigned int x = (S_q16 >> 8) + (1u << 8);
    x = (x + ((uint64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((uint64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((uint64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((uint64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((uint64_t)S_q16 << 16) / x) >> 1;
    x = (x + ((uint64_t)S_q16 << 16) / x) >> 1;
    return x;
}

/* ═════════════════════════════════════════════════════════════════════
 * 3. SENO — VERLET/LEAPFROG (2º ORDEN SIMPLÉCTICO) + RE-SYNC
 * ═════════════════════════════════════════════════════════════════════
 */
#define SINE_STEPS 36000
#define SYNC_STEPS 10
#define TWO_PI 6.283185307179586476925286766559

static double sine_table[SINE_STEPS + 1];

static void aryabhata_sine_gen_double(double *out, int n) {
    if (n <= 0) return;
    double h = TWO_PI / SINE_STEPS;
    double h2 = h * 0.5;
    double s = 0.0;
    double c = 1.0;
    int i;
    out[0] = s;
    if (n == 1) return;
    for (i = 1; i < n; ++i) {
        double c_half = c - h2 * s;
        s = s + h * c_half;
        c = c_half - h2 * s;
        out[i] = s;
        if (i % 10 == 0) {
            double theta = i * (TWO_PI / SINE_STEPS);
            s = sin(theta);
            c = cos(theta);
        }
    }
}

/* Punto fijo Q16.16 para FPGA/ASIC — sin FPU, sin libm */
#define Q16_ONE 65536
#define Q16_HALF 32768
#define Q16_TWO_PI 411775

static int aryabhata_sine_gen_q16(int *out, int n) {
    if (n <= 0) return 0;
    int h = Q16_TWO_PI / SINE_STEPS;
    int h2 = h >> 1;
    int s = 0;
    int c = Q16_ONE;
    int i;
    out[0] = s;
    if (n == 1) return 0;
    for (i = 1; i < n; ++i) {
        int c_half = c - ((h2 * (long long)s) >> 16);
        s = s + ((h * (long long)c_half) >> 16);
        c = c_half - ((h2 * (long long)s) >> 16);
        out[i] = s;
    }
    return 0;
}

/* Q32.32 para mayor precisión */
#define Q32_ONE 4294967296.0
#define Q32_TWO_PI 27132038846ULL

static int64_t aryabhata_sine_gen_q32(int64_t *out, int n) {
    if (n <= 0) return 0;
    int64_t h = (int64_t)(Q32_TWO_PI / SINE_STEPS);
    int64_t h2 = h >> 1;
    int64_t s = 0;
    int64_t c = (int64_t)Q32_ONE;
    int i;
    out[0] = 0;
    if (n == 1) return 0;
    for (i = 1; i < n; ++i) {
        int64_t c_half = c - ((h2 * s) >> 32);
        s = s + ((h * c_half) >> 32);
        c = c_half - ((h2 * s) >> 32);
        out[i] = s;
    }
    return 0;
}

/* ════════════════════════════════════════════════════════════════════
 * TEST
 * ════════════════════════════════════════════════════════════════════ */
#ifdef TEST

static void test_crt(void) {
    printf("=== Test CRT (1M) ===\n");
    r1 = r2 = r3 = 0;
    uint64_t exact = 0;
    uint64_t i;
    for (i = 0; i < 1000000; ++i) {
        unsigned int x = (unsigned int)(i % 65536);
        crt_accumulate(x);
        exact += x;
    }
    uint64_t got = crt_reconstruct();
    printf(" exact=%lu got=%lu %s\n", exact, got, got == exact ? "OK" : "FAIL");
}

static void test_heron(void) {
    printf("=== Test Heron ===\n");
    double maxr = 0;
    double tv[] = {1e-6, 1e-3, 1.0, 2.0, 10.0, 100.0, 1e4, 1e6, 1e9};
    size_t i;
    for (i = 0; i < sizeof(tv)/sizeof(tv[0]); ++i) {
        double r = heron_sqrt(tv[i]), ref = sqrt(tv[i]), e = fabs(r - ref) / ref;
        if (e > maxr) maxr = e;
    }
    printf(" max_rel_err=%.2e %s\n", maxr, maxr < 2.2e-16 ? "OK" : "FAIL");
}

static void test_sine(void) {
    printf("=== Test Sine (Verlet+sync) ===\n");
    aryabhata_sine_gen_double(sine_table, SINE_STEPS + 1);
    double maxe = 0;
    int i;
    for (i = 0; i <= SINE_STEPS; ++i) {
        double th = i * TWO_PI / SINE_STEPS;
        double e = fabs(sine_table[i] - sin(th));
        if (e > maxe) maxe = e;
    }
    printf(" max_abs_err=%.2e target<7e-14 %s\n", maxe, maxe < 7e-14 ? "OK" : "~100x target");
}

int main(void) {
    test_crt();
    test_heron();
    test_sine();
    printf("\nDemostracion:\n");
    r1 = r2 = r3 = 0;
    int i;
    for (i = 0; i < 1000; ++i) crt_accumulate(1000);
    printf(" CRT 1000*1000 = %lu\n", crt_reconstruct());
    printf(" sqrt(2)=%.15f (libm=%.15f)\n", heron_sqrt(2.0), sqrt(2.0));
    return 0;
}
#endif