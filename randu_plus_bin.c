#include <stdio.h>
#include <stdint.h>
static inline uint32_t randu_plus(uint32_t *s) {
    *s = 65539u * *s;
    uint32_t x = *s;
    x ^= x >> 16; x *= 0x85ebca6bu;
    x ^= x >> 13; x *= 0xc2b2ae35u;
    x ^= x >> 16;
    return x;
}
int main(int argc, char** argv) {
    uint32_t s = (argc > 1) ? (uint32_t)strtoul(argv[1], NULL, 10) : 12345u;
    size_t n = (argc > 2) ? strtoul(argv[2], NULL, 10) : 1000000;
    for (size_t i = 0; i < n; ++i) {
        uint32_t r = randu_plus(&s);
        fwrite(&r, 4, 1, stdout);
    }
    return 0;
}