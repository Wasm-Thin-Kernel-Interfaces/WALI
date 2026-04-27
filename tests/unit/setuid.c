// CMD: args="self"
// CMD: args="bad_uid"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setuid")))
long __imported_wali_setuid(uid_t uid);
__attribute__((__import_module__("wali"), __import_name__("SYS_getuid")))
long __imported_wali_getuid(void);
int wali_setuid(uid_t u) { return (int)__imported_wali_setuid(u); }
uid_t wali_getuid(void) { return (uid_t)__imported_wali_getuid(); }
#else
#include <sys/syscall.h>
int wali_setuid(uid_t u) { return syscall(SYS_setuid, u); }
uid_t wali_getuid(void) { return syscall(SYS_getuid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self";

    if (!strcmp(mode, "self"))    return (wali_setuid(wali_getuid()) == 0) ? 0 : -1;
    if (!strcmp(mode, "bad_uid")) {
        long r = wali_setuid(65530);
        return (r == 0) ? 0 : 1;
    }
    return -1;
}
