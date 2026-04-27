// CMD: args="basic"
// CMD: args="idempotent"

#include "wali_start.c"
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_set_tid_address")))
long __imported_wali_set_tid_address(int *tidptr);
long wali_set_tid_address(int *tidptr) { return __imported_wali_set_tid_address(tidptr); }
#else
#include <sys/syscall.h>
long wali_set_tid_address(int *tidptr) { return syscall(SYS_set_tid_address, tidptr); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    int storage = 0;
    long tid = wali_set_tid_address(&storage);
    if (tid <= 0) return -1;

    if (!strcmp(mode, "idempotent")) {
        // Calling again must return the same tid.
        long tid2 = wali_set_tid_address(&storage);
        return (tid == tid2) ? 0 : -1;
    }
    return 0;
}
