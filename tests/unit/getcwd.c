// CMD: args="ok"
// CMD: args="small_buf"

#include "wali_start.c"
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getcwd")))
long __imported_wali_getcwd(char *buf, size_t size);
long wali_getcwd(char *buf, size_t size) { return __imported_wali_getcwd(buf, size); }
#else
#include <sys/syscall.h>
long wali_getcwd(char *buf, size_t size) { return syscall(SYS_getcwd, buf, size); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    if (!strcmp(mode, "ok")) {
        char buf[256];
        long r = wali_getcwd(buf, sizeof(buf));
        return (r > 0 && buf[0] == '/') ? 0 : -1;
    }
    if (!strcmp(mode, "small_buf")) {
        char buf[2];
        long r = wali_getcwd(buf, sizeof(buf));
        return (r < 0) ? 0 : -1;  // ERANGE for any non-trivial cwd
    }
    return -1;
}
