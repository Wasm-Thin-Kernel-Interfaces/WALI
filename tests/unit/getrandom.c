// CMD: args="small"
// CMD: args="big"
// CMD: args="nonblock"
// CMD: args="zero_size"
// CMD: args="bad_flags"

#include "wali_start.c"
#include <stddef.h>
#include <string.h>

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x0001
#endif
#ifndef GRND_RANDOM
#define GRND_RANDOM 0x0002
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getrandom")))
long __imported_wali_getrandom(void *buf, size_t buflen, unsigned int flags);
int wali_getrandom(void *buf, size_t buflen, unsigned int flags) {
    return (int)__imported_wali_getrandom(buf, buflen, flags);
}
#else
#include <sys/syscall.h>
#ifndef SYS_getrandom
#define SYS_getrandom 318
#endif
int wali_getrandom(void *buf, size_t buflen, unsigned int flags) {
    return syscall(SYS_getrandom, buf, buflen, flags);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "small";

    if (!strcmp(mode, "zero_size")) {
        char buf[1];
        long r = wali_getrandom(buf, 0, 0);
        return (r == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_flags")) {
        char buf[16];
        long r = wali_getrandom(buf, sizeof(buf), 0xFFFFFFFF);
        return (r < 0) ? 0 : -1;
    }

    size_t want;
    unsigned int flags = 0;
    if (!strcmp(mode, "small"))         { want = 32;  flags = 0; }
    else if (!strcmp(mode, "big"))      { want = 256; flags = 0; }
    else if (!strcmp(mode, "nonblock")) { want = 16;  flags = GRND_NONBLOCK; }
    else return -1;

    char buf[256];
    memset(buf, 0, sizeof(buf));
    long r = wali_getrandom(buf, want, flags);
    if (r != (long)want) return -1;

    // Buffer should have non-zero content (probability of all zeros is ~2^-256 for n=32).
    int all_zero = 1;
    for (size_t i = 0; i < want; i++) if (buf[i] != 0) { all_zero = 0; break; }
    return all_zero ? -1 : 0;
}
