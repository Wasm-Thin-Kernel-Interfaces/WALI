// CMD: setup="symlink:/tmp/rl_a /tmp/rl_a_target" args="ok          /tmp/rl_a /tmp/rl_a_target" cleanup="/tmp/rl_a /tmp/rl_a_target"
// CMD:                                              args="missing     /tmp/rl_no /tmp/rl_no"      cleanup=""
// CMD: setup="file:/tmp/rl_b"                      args="not_a_link  /tmp/rl_b /tmp/rl_b"        cleanup="/tmp/rl_b"
// CMD: setup="symlink:/tmp/rl_c /tmp/rl_c_target" args="small_buf   /tmp/rl_c /tmp/rl_c_target" cleanup="/tmp/rl_c /tmp/rl_c_target"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_readlink")))
long __imported_wali_readlink(const char *pathname, char *buf, size_t bufsiz);
long wali_readlink(const char *pathname, char *buf, size_t bufsiz) { return (long)__imported_wali_readlink(pathname, buf, bufsiz); }
#else
#include <sys/syscall.h>
long wali_readlink(const char *pathname, char *buf, size_t bufsiz) {
#ifdef SYS_readlink
    return syscall(SYS_readlink, pathname, buf, bufsiz);
#else
    return syscall(SYS_readlinkat, AT_FDCWD, pathname, buf, bufsiz);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "file:", 5)) {
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
    } else if (!strncmp(argv[0], "symlink:", 8)) {
        if (argc < 2) return -1;
        const char *link = argv[0] + 8;
        const char *target = argv[1];
        int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
        unlink(link);
        return symlink(target, link);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    for (int i = 0; i < argc; i++) unlink(argv[i]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 4) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];
    const char *target = argv[3];

    if (!strcmp(mode, "ok")) {
        char buf[64];
        long r = wali_readlink(path, buf, sizeof(buf));
        if (r < 0 || r >= (long)sizeof(buf)) return -1;
        buf[r] = '\0';
        return (strcmp(buf, target) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "missing")) {
        char buf[64];
        long r = wali_readlink(path, buf, sizeof(buf));
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "not_a_link")) {
        char buf[64];
        long r = wali_readlink(path, buf, sizeof(buf));
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "small_buf")) {
        // bufsiz=2 → readlink truncates and returns 2 (no null).
        char buf[2];
        long r = wali_readlink(path, buf, sizeof(buf));
        return (r == 2) ? 0 : -1;
    }
    return -1;
}
