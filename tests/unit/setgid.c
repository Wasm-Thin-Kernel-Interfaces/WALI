// CMD: args="self"
// CMD: args="bad_gid"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setgid")))
long __imported_wali_setgid(gid_t gid);
__attribute__((__import_module__("wali"), __import_name__("SYS_getgid")))
long __imported_wali_getgid(void);
int wali_setgid(gid_t g) { return (int)__imported_wali_setgid(g); }
gid_t wali_getgid(void) { return (gid_t)__imported_wali_getgid(); }
#else
#include <sys/syscall.h>
int wali_setgid(gid_t g) { return syscall(SYS_setgid, g); }
gid_t wali_getgid(void) { return syscall(SYS_getgid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self";

    if (!strcmp(mode, "self")) {
        // Setting to the current gid is always permitted.
        return (wali_setgid(wali_getgid()) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_gid")) {
        // Setting to a foreign gid fails for non-root; both targets must
        // agree on the failure.
        long r = wali_setgid(65530);
        return (r == 0) ? 0 : 1;
    }
    return -1;
}
