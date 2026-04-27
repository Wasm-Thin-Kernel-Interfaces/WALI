// CMD: args="noop"
// CMD: args="self"
// CMD: args="bad_uid"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setresuid")))
long __imported_wali_setresuid(uid_t ruid, uid_t euid, uid_t suid);
__attribute__((__import_module__("wali"), __import_name__("SYS_getuid")))
long __imported_wali_getuid(void);
__attribute__((__import_module__("wali"), __import_name__("SYS_geteuid")))
long __imported_wali_geteuid(void);
int wali_setresuid(uid_t r, uid_t e, uid_t s) { return (int)__imported_wali_setresuid(r, e, s); }
uid_t wali_getuid(void) { return (uid_t)__imported_wali_getuid(); }
uid_t wali_geteuid(void) { return (uid_t)__imported_wali_geteuid(); }
#else
#include <sys/syscall.h>
int wali_setresuid(uid_t r, uid_t e, uid_t s) { return syscall(SYS_setresuid, r, e, s); }
uid_t wali_getuid(void) { return syscall(SYS_getuid); }
uid_t wali_geteuid(void) { return syscall(SYS_geteuid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "noop";

    if (!strcmp(mode, "noop"))  return (wali_setresuid(-1, -1, -1) == 0) ? 0 : -1;
    if (!strcmp(mode, "self")) {
        uid_t u = wali_getuid();
        uid_t eu = wali_geteuid();
        return (wali_setresuid(u, eu, eu) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_uid")) {
        long r = wali_setresuid(65530, 65530, 65530);
        return (r == 0) ? 0 : 1;
    }
    return -1;
}
