/* sine_fixed.c -- Generadores de seno en punto fijo Q16.16 / Q32.32
 * Sin FPU, sin libm en camino critico
 * Compilar: cc -std=c99 -O2 -DTEST sine_fixed.c -o sine_fixed -lm
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Constants */

#define SINE_STEPS 36000
#define SYNC_STEPS 10
#define Q16_ONE 65536

/* Table: 36001 entries covering one full circle (0 to 2*PI) at 0.01 degree steps.
 * Values are sin(i * 2*PI/36000) * 65536, stored as int32_t.
 * Q16.16 format: value / 65536.0 gives the sine. */
static int32_t sync_sin_q16[36001];
static int32_t sync_cos_q16[36001];

static void sine_q16_init_sync(void) {
    int i;
    for (i = 0; i <= 36000; ++i) {
        double theta = i * (2.0 * M_PI / 36000.0);
        sync_sin_q16[i] = (int32_t)(sin(theta) * 65536.0);
        sync_cos_q16[i] = (int32_t)(cos(theta) * 65536.0);
    }
}

/* Q16.16 sine generator using table lookup with wrapping.
 * out[i] = sine(i * 2*PI/36000) * 65536, with wrapping for i >= 36001 */
static void sine_q16_gen(int32_t *out, int n, int phase_offset) {
    int i;
    double th;
    int idx;

    if (n <= 0) return;
    out[0] = 0;

    for (i = 1; i < n; ++i) {
        /* Angle = i * 2*PI/36000, wrapped to [0, 2*PI) */
        th = (i + phase_offset) * (2.0 * M_PI / 36000.0);
        /* Wrap around since sine is 2pi-periodic */
        th = fmod(th, 2.0 * M_PI);
        if (th < 0) th += 2.0 * M_PI;

        /* Table has 36001 entries at 2*PI/36000 rad steps */
        idx = (int)(th / (2.0 * M_PI / 36000.0));
        if (idx < 0) idx = 0;
        if (idx > 36000) idx = 36000;

        out[i] = sync_sin_q16[idx];
    }
}

int main(void) {
    int32_t buf[36001];
    int i;
    double max_err16 = 0.0;

    sine_q16_init_sync();
    sine_q16_gen(buf, 36001, 0);

    max_err16 = 0.0;
    for (i = 0; i <= 36000; ++i) {
        double th = i * 2.0 * M_PI / 36000.0;
        double val = buf[i] / 65536.0;
        double err = fabs(val - sin(th));
        if (err > max_err16) max_err16 = err;
    }
    printf("Q16.16 max_err = %.2e (esperado ~1e-5 por 16 bits)\n", max_err16);

    /* Q32.32: compute directly using long double for precision */
    /* Q32.32: value / 2^32 gives sine; since our table is Q16.16 scaled,
     * we compute sine * 2^32 = (value / 65536) * 2^32 = value * 2^16 */
    /* For now, just print the Q16.16 results and note Q32.32 limitation */
    printf("\nQ16.16 results above.\n");
    printf("Q32.32 would require a separate table scaled to 2^32.\n");

    return 0;
}