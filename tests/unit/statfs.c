// CMD: args="cwd ."
// CMD: args="root /"
// CMD: args="tmp /tmp"
// CMD: args="missing /tmp/statfs_no_such_path"

#include "wali_start.c"
#include <sys/statfs.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_statfs")))
long __imported_wali_statfs(const char *path, struct statfs *buf);
int wali_statfs(const char *p, struct statfs *b) { return (int)__imported_wali_statfs(p, b); }
#else
#include <sys/syscall.h>
int wali_statfs(const char *p, struct statfs *b) { return syscall(SYS_statfs, p, b); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    struct statfs st;
    long r = wali_statfs(path, &st);
    if (!strcmp(mode, "missing")) return (r < 0) ? 0 : -1;
    if (r != 0) return -1;
    return (st.f_type != 0) ? 0 : -1;
}
