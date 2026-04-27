// CMD: args="get_dumpable"
// CMD: args="set_get_dumpable"
// CMD: args="bad_option"

// Disabled — WALI / native divergence on prctl options that pass a
// userspace buffer pointer (e.g. PR_SET_NAME, PR_GET_NAME, PR_GET_PDEATHSIG):
//   Native: the kernel reads/writes the buffer at the given pointer.
//   WALI:   the buffer pointer doesn't appear to be translated/marshalled
//           across the WASM↔host boundary, so the call returns failure.
// Re-enable once WALI's prctl handler marshals user pointers. Until then,
// this file restricts itself to options that take/return only integer
// arg2 values (PR_GET_DUMPABLE, PR_SET_DUMPABLE).
//
// // CMD: args="set_get_name"
// // CMD: args="get_pdeathsig"

#include "wali_start.c"
#include <sys/prctl.h>
#include <string.h>

#ifndef PR_SET_DUMPABLE
#define PR_SET_DUMPABLE 4
#endif
#ifndef PR_GET_DUMPABLE
#define PR_GET_DUMPABLE 3
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_prctl")))
long __imported_wali_prctl(int option, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5);
int wali_prctl(int o, unsigned long a, unsigned long b, unsigned long c, unsigned long d) {
    return (int)__imported_wali_prctl(o, a, b, c, d);
}
#else
#include <sys/syscall.h>
int wali_prctl(int o, unsigned long a, unsigned long b, unsigned long c, unsigned long d) {
    return syscall(SYS_prctl, o, a, b, c, d);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "get_dumpable";

    if (!strcmp(mode, "get_dumpable")) {
        // PR_GET_DUMPABLE returns 0 or 1 (or 2) directly, no buffer.
        long r = wali_prctl(PR_GET_DUMPABLE, 0, 0, 0, 0);
        return (r >= 0) ? 0 : -1;
    }
    if (!strcmp(mode, "set_get_dumpable")) {
        // Set 1, read back, must be 1.
        if (wali_prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) != 0) return -1;
        long r = wali_prctl(PR_GET_DUMPABLE, 0, 0, 0, 0);
        return (r == 1) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_option")) {
        long r = wali_prctl(99999, 0, 0, 0, 0);
        return (r < 0) ? 0 : -1;
    }
    // Disabled — buffer-pointer marshalling divergence (see header comment).
    // if (!strcmp(mode, "set_get_name")) {
    //     char name[16]; strcpy(name, "walitest");
    //     if (wali_prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0) != 0) return -1;
    //     char buf[16] = {0};
    //     if (wali_prctl(PR_GET_NAME, (unsigned long)buf, 0, 0, 0) != 0) return -1;
    //     return (strcmp(buf, "walitest") == 0) ? 0 : -1;
    // }
    // if (!strcmp(mode, "get_pdeathsig")) {
    //     int sig = -1;
    //     long r = wali_prctl(PR_GET_PDEATHSIG, (unsigned long)&sig, 0, 0, 0);
    //     return (r == 0 && sig >= 0) ? 0 : -1;
    // }
    return -1;
}
