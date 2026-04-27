// CMD: args="basic"
// CMD: args="getsid_works"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setsid")))
long __imported_wali_setsid(void);
__attribute__((__import_module__("wali"), __import_name__("SYS_getsid")))
long __imported_wali_getsid(pid_t pid);
int wali_setsid(void) { return (int)__imported_wali_setsid(); }
int wali_getsid(pid_t p) { return (int)__imported_wali_getsid(p); }
#else
#include <sys/syscall.h>
int wali_setsid(void) { return syscall(SYS_setsid); }
int wali_getsid(pid_t p) { return syscall(SYS_getsid, p); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    if (!strcmp(mode, "basic")) {
        // setsid usually fails with EPERM (we're a process group leader).
        // Just exercise the syscall — both targets should agree.
        long r = wali_setsid();
        return (r >= 0) ? 0 : 1;
    }
    if (!strcmp(mode, "getsid_works")) {
        // After whatever setsid did, getsid(0) must still return a valid sid.
        wali_setsid();
        return (wali_getsid(0) >= 0) ? 0 : -1;
    }
    return -1;
}
