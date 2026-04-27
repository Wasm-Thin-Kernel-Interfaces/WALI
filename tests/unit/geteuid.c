// CMD: args="basic"
// CMD: args="idempotent"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_geteuid")))
long __imported_wali_geteuid(void);
uid_t wali_geteuid(void) { return (uid_t)__imported_wali_geteuid(); }
#else
#include <sys/syscall.h>
uid_t wali_geteuid(void) { return syscall(SYS_geteuid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "basic")) return (wali_geteuid() != (uid_t)-1) ? 0 : -1;
    if (!strcmp(mode, "idempotent")) {
        uid_t a = wali_geteuid();
        uid_t b = wali_geteuid();
        return (a == b) ? 0 : -1;
    }
    return -1;
}
