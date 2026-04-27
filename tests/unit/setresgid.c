// CMD: args="noop"
// CMD: args="self"
// CMD: args="bad_gid"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setresgid")))
long __imported_wali_setresgid(gid_t rgid, gid_t egid, gid_t sgid);
__attribute__((__import_module__("wali"), __import_name__("SYS_getgid")))
long __imported_wali_getgid(void);
__attribute__((__import_module__("wali"), __import_name__("SYS_getegid")))
long __imported_wali_getegid(void);
int wali_setresgid(gid_t r, gid_t e, gid_t s) { return (int)__imported_wali_setresgid(r, e, s); }
gid_t wali_getgid(void) { return (gid_t)__imported_wali_getgid(); }
gid_t wali_getegid(void) { return (gid_t)__imported_wali_getegid(); }
#else
#include <sys/syscall.h>
int wali_setresgid(gid_t r, gid_t e, gid_t s) { return syscall(SYS_setresgid, r, e, s); }
gid_t wali_getgid(void) { return syscall(SYS_getgid); }
gid_t wali_getegid(void) { return syscall(SYS_getegid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "noop";

    if (!strcmp(mode, "noop"))  return (wali_setresgid(-1, -1, -1) == 0) ? 0 : -1;
    if (!strcmp(mode, "self")) {
        gid_t g = wali_getgid();
        gid_t eg = wali_getegid();
        return (wali_setresgid(g, eg, eg) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_gid")) {
        long r = wali_setresgid(65530, 65530, 65530);
        return (r == 0) ? 0 : 1;
    }
    return -1;
}
