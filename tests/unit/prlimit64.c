// CMD: args="get_nofile"
// CMD: args="set_then_get"
// CMD: args="bad_resource"
// CMD: args="self_explicit_pid"

#include "wali_start.c"
#include <sys/resource.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_prlimit64")))
long __imported_wali_prlimit64(int pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);
int wali_prlimit64(int pid, int resource, const struct rlimit *nl, struct rlimit *ol) {
    return (int)__imported_wali_prlimit64(pid, resource, nl, ol);
}
int wali_getpid(void) { return (int)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
int wali_prlimit64(int pid, int resource, const struct rlimit *nl, struct rlimit *ol) {
    return syscall(SYS_prlimit64, pid, resource, nl, ol);
}
int wali_getpid(void) { return syscall(SYS_getpid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "get_nofile";

    if (!strcmp(mode, "get_nofile")) {
        struct rlimit old;
        if (wali_prlimit64(0, RLIMIT_NOFILE, NULL, &old) != 0) return -1;
        return (old.rlim_cur > 0 && old.rlim_cur <= old.rlim_max) ? 0 : -1;
    }
    if (!strcmp(mode, "self_explicit_pid")) {
        // pid=0 vs pid=getpid() must yield identical limits.
        struct rlimit a, b;
        if (wali_prlimit64(0, RLIMIT_STACK, NULL, &a) != 0) return -1;
        if (wali_prlimit64(wali_getpid(), RLIMIT_STACK, NULL, &b) != 0) return -1;
        return (a.rlim_cur == b.rlim_cur && a.rlim_max == b.rlim_max) ? 0 : -1;
    }
    if (!strcmp(mode, "set_then_get")) {
        // Read current, set soft to current (no-op change), read back.
        struct rlimit current;
        if (wali_prlimit64(0, RLIMIT_NOFILE, NULL, &current) != 0) return -1;
        struct rlimit new_lim = current;
        if (new_lim.rlim_cur > new_lim.rlim_max) new_lim.rlim_cur = new_lim.rlim_max;
        if (wali_prlimit64(0, RLIMIT_NOFILE, &new_lim, NULL) != 0) return -1;
        struct rlimit after;
        if (wali_prlimit64(0, RLIMIT_NOFILE, NULL, &after) != 0) return -1;
        return (after.rlim_cur == new_lim.rlim_cur) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_resource")) {
        struct rlimit old;
        long r = wali_prlimit64(0, 9999, NULL, &old);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
