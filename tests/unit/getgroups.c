// CMD: args="query"
// CMD: args="list"
// CMD: args="too_small"

#include "wali_start.c"
#include <sys/types.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getgroups")))
long __imported_wali_getgroups(int size, gid_t *list);
int wali_getgroups(int size, gid_t *list) { return (int)__imported_wali_getgroups(size, list); }
#else
#include <sys/syscall.h>
int wali_getgroups(int size, gid_t *list) { return syscall(SYS_getgroups, size, list); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "query";

    if (!strcmp(mode, "query")) {
        // size=0 returns count without writing list.
        int n = wali_getgroups(0, NULL);
        return (n >= 0) ? 0 : -1;
    }
    if (!strcmp(mode, "list")) {
        gid_t buf[128];
        int n = wali_getgroups(128, buf);
        if (n < 0) return -1;
        // Re-query with size=0 must give same count.
        int m = wali_getgroups(0, NULL);
        return (n == m) ? 0 : -1;
    }
    if (!strcmp(mode, "too_small")) {
        // Get true count.
        int n = wali_getgroups(0, NULL);
        if (n <= 0) return 0;  // No groups → can't trigger EINVAL meaningfully.
        gid_t buf[128];
        int r = wali_getgroups(n - 1, buf);
        return (r < 0) ? 0 : -1;  // EINVAL when buf is too small.
    }
    return -1;
}
