// CMD: args="basic"
// CMD: args="repeated"

#include "wali_start.c"
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_sched_yield")))
long __imported_wali_sched_yield(void);
int wali_sched_yield(void) { return (int)__imported_wali_sched_yield(); }
#else
#include <sys/syscall.h>
int wali_sched_yield(void) { return syscall(SYS_sched_yield); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "basic"))    return (wali_sched_yield() == 0) ? 0 : -1;
    if (!strcmp(mode, "repeated")) {
        // Yielding repeatedly must always return 0.
        for (int i = 0; i < 5; i++) if (wali_sched_yield() != 0) return -1;
        return 0;
    }
    return -1;
}
