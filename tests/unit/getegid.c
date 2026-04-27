// CMD: args="basic"
// CMD: args="idempotent"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getegid")))
long __imported_wali_getegid(void);
gid_t wali_getegid(void) { return (gid_t)__imported_wali_getegid(); }
#else
#include <sys/syscall.h>
gid_t wali_getegid(void) { return syscall(SYS_getegid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "basic")) {
        return (wali_getegid() != (gid_t)-1) ? 0 : -1;
    }
    if (!strcmp(mode, "idempotent")) {
        // Two calls in succession must return the same value.
        gid_t a = wali_getegid();
        gid_t b = wali_getegid();
        return (a == b) ? 0 : -1;
    }
    return -1;
}
