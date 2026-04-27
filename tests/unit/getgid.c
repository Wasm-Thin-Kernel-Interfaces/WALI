// CMD: args="basic"
// CMD: args="idempotent"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getgid")))
long __imported_wali_getgid(void);
gid_t wali_getgid(void) { return (gid_t)__imported_wali_getgid(); }
#else
#include <sys/syscall.h>
gid_t wali_getgid(void) { return syscall(SYS_getgid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "basic")) return (wali_getgid() != (gid_t)-1) ? 0 : -1;
    if (!strcmp(mode, "idempotent")) {
        gid_t a = wali_getgid();
        gid_t b = wali_getgid();
        return (a == b) ? 0 : -1;
    }
    return -1;
}
