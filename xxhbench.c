#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"

enum xxh_alg_enum {
    xxh_alg_32,
    xxh_alg_32u,
    xxh_alg_64,
    xxh_alg_64u
};

static const size_t g_bufsz = 100*1024;
static const int g_iters = 3;

static clock_t clock_span(clock_t start)
{
    return clock() - start;
}

static uint32_t bench_xxh32(const void *buf, size_t len, uint32_t seed)
{
    return XXH32(buf, len, seed);
}

static uint32_t bench_xxh64(const void *buf, size_t len, uint32_t seed)
{
    return XXH64(buf, len, seed);
}

static void bench_xxh(uint32_t (*h)(const void *, size_t, uint32_t),
        const char* name, const void* buf, size_t len)
{
    int i, perloop = 100;
    double times, fastest = 100000000.;

    printf("\r%79s\r", "");
    for (i = 1; i <= g_iters; i++) {
        uint32_t count = 0, r = 0;
        clock_t start;

        printf("%1i-%-17.17s : %10u ->\r", i, name, (uint32_t)len);
        start = clock();
        while (clock() == start);
        start = clock();

        while (clock_span(start) < CLOCKS_PER_SEC) {
            int j;
            for (j = 0; j < perloop; j++)
                r += h(buf, len, j);
            count += perloop;
        }
        /* to avoid compiler optimizing away hash function */
        if (r == 0)
            printf(".");

        times = ((double)clock_span(start)/CLOCKS_PER_SEC)/count;
        if (times < fastest)
            fastest = times;
        printf("%1i-%-17.17s : %10u -> %7.1f MB/s\r",
                i, name, (uint32_t)len, ((double)len/(1<<20))/fastest);
    }
    printf("%-19.19s : %10u -> %7.1f MB/s  \n", name, (uint32_t)len,
            ((double)len/(1<<20))/fastest);
}

static int bench(enum xxh_alg_enum alg)
{
    /* Extra bytes for unaligned test */
    void *buf = malloc(g_bufsz+3);

    if (!buf) {
        printf("\nError: not enough memory!\n");
        return 1;
    }
    memset(buf, 0x5A, g_bufsz);

    printf("\rSample of %u KB...\n", (uint32_t)(g_bufsz >> 10));
    switch (alg) {
    case xxh_alg_32:
        bench_xxh(bench_xxh32, "XXH32", buf, g_bufsz);
        break;
    case xxh_alg_32u:
        bench_xxh(bench_xxh32, "XXH32 unaligned", buf+1, g_bufsz);
        break;
    case xxh_alg_64:
        bench_xxh(bench_xxh64, "XXH64", buf, g_bufsz);
        break;
    case xxh_alg_64u:
        bench_xxh(bench_xxh64, "XXH64 unaligned", buf+3, g_bufsz);
        break;
    }

    free(buf);
    return 0;
}

static void check_result(uint64_t r1, uint64_t r2)
{
    if (r1 != r2) {
        printf("\nERROR : 64-bits values non equals   !!!!!   \n");
        printf("\n %" PRIu64 "!= %" PRIu64 "\n", r1, r2);
        exit(1);
    }
}

static void check32(const void* buf, size_t len, uint32_t seed, uint32_t truth)
{
    XXH32_state_t state;
    uint32_t hash;
    size_t pos;

    hash = XXH32(buf, len, seed);
    check_result(hash, truth);

    XXH32_reset(&state, seed);
    XXH32_update(&state, buf, len);
    hash = XXH32_digest(&state);
    check_result(hash, truth);

    XXH32_reset(&state, seed);
    for (pos=0; pos<len; pos++)
        XXH32_update(&state, buf+pos, 1);
    hash = XXH32_digest(&state);
    check_result(hash, truth);
}

static void check64(const void* buf, size_t len, uint64_t seed, uint64_t truth)
{
    XXH64_state_t state;
    uint64_t hash;
    size_t pos;

    hash = XXH64(buf, len, seed);
    check_result(hash, truth);

    XXH64_reset(&state, seed);
    XXH64_update(&state, buf, len);
    hash = XXH64_digest(&state);
    check_result(hash, truth);

    XXH64_reset(&state, seed);
    for (pos=0; pos<len; pos++)
        XXH64_update(&state, buf+pos, 1);
    hash = XXH64_digest(&state);
    check_result(hash, truth);
}

static void check(void)
{
    #define BUFSZ 101
    uint8_t buf[BUFSZ];

    int i;
    const uint32_t prime = 2654435761U;
    uint32_t byte_gen = prime;

    for (i = 0; i < BUFSZ; i++) {
        buf[i] = byte_gen >> 24;
        byte_gen *= byte_gen;
    }

    check32(NULL, 0,     0,     0x02CC5D05);
    check32(NULL, 0,     prime, 0x36B78AE7);
    check32(buf,  1,     0,     0xB85CBEE5);
    check32(buf,  1,     prime, 0xD5845D64);
    check32(buf,  14,    0,     0xE5AA0AB4);
    check32(buf,  14,    prime, 0x4481951D);
    check32(buf,  BUFSZ, 0,     0x1F1AA412);
    check32(buf,  BUFSZ, prime, 0x498EC8E2);

    check64(NULL, 0,     0,     0xEF46DB3751D8E999ULL);
    check64(NULL, 0,     prime, 0xAC75FDA2929B17EFULL);
    check64(buf,  1,     0,     0x4FCE394CC88952D8ULL);
    check64(buf,  1,     prime, 0x739840CB819FA723ULL);
    check64(buf,  14,    0,     0xCFFA8DB881BC3A3DULL);
    check64(buf,  14,    prime, 0x5B9611585EFCC9CBULL);
    check64(buf,  BUFSZ, 0,     0x0EAB543384F878ADULL);
    check64(buf,  BUFSZ, prime, 0xCAA65939306F1E21ULL);

    printf("Sanity check -- all tests ok\n");
}

static int usage(const char *exename)
{
    printf("Usage: %s [arg]\n", exename);
    printf("Arguments :\n");
    printf("  check - validate checksum result\n");
    printf("  32    - benchmark 32bits aligned\n");
    printf("  32u   - benchmark 32bits unaligned\n");
    printf("  64    - benchmark 64bits aligned\n");
    printf("  64u   - benchmark 64bits unaligned\n");
    return 1;
}

int main(int argc, const char* argv[])
{
    const char *param;

    if (argc != 2)
        return usage(argv[0]);

    param = argv[1];
    if (strcmp(param, "check") == 0)
        check();
    else if (strcmp(param, "32") == 0)
        bench(xxh_alg_32);
    else if (strcmp(param, "32u") == 0)
        bench(xxh_alg_32u);
    else if (strcmp(param, "64") == 0)
        bench(xxh_alg_64);
    else if (strcmp(param, "64u") == 0)
        bench(xxh_alg_64u);
    else
        return usage(argv[0]);

    return 0;
}
