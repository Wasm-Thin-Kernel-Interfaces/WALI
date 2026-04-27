// CMD: args="self_egid"
// CMD: args="empty"
// CMD: args="bad_size"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setgroups")))
long __imported_wali_setgroups(int size, const gid_t *list);
__attribute__((__import_module__("wali"), __import_name__("SYS_getegid")))
long __imported_wali_getegid(void);
int wali_setgroups(int s, const gid_t *l) { return (int)__imported_wali_setgroups(s, l); }
gid_t wali_getegid(void) { return (gid_t)__imported_wali_getegid(); }
#else
#include <sys/syscall.h>
int wali_setgroups(int s, const gid_t *l) { return syscall(SYS_setgroups, s, l); }
gid_t wali_getegid(void) { return syscall(SYS_getegid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self_egid";

    // Outcome depends on root/non-root; the differential check verifies
    // WALI matches native regardless of context.
    if (!strcmp(mode, "self_egid")) {
        gid_t g[1] = { wali_getegid() };
        long r = wali_setgroups(1, g);
        return (r == 0) ? 0 : 1;
    }
    if (!strcmp(mode, "empty")) {
        long r = wali_setgroups(0, NULL);
        return (r == 0) ? 0 : 1;
    }
    if (!strcmp(mode, "bad_size")) {
        // Negative size → EINVAL.
        gid_t g[1] = { 0 };
        long r = wali_setgroups(-1, g);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
