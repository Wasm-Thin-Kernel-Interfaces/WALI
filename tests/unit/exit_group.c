// CMD: args="0"
// CMD: args="42"
// CMD: args="255"

#include "wali_start.c"
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_exit_group")))
long __imported_wali_exit_group(int status);
void wali_exit_group(int status) { __imported_wali_exit_group(status); }
#else
#include <sys/syscall.h>
void wali_exit_group(int status) { syscall(SYS_exit_group, status); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

static int parse_int(const char *s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 2) return -1;
    int code = parse_int(argv[1]);

    wali_exit_group(code);
    // Should not reach here. If we do, the syscall didn't terminate the process.
    return 99;
}
