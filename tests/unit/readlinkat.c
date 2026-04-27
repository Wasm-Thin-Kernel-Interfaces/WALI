// CMD: setup="symlink:/tmp/rla_a /tmp/rla_a_target" args="ok       /tmp/rla_a /tmp/rla_a_target" cleanup="/tmp/rla_a /tmp/rla_a_target"
// CMD:                                                args="missing  /tmp/rla_no /tmp/rla_no"     cleanup=""
// CMD: setup="file:/tmp/rla_b"                       args="not_link /tmp/rla_b /tmp/rla_b"        cleanup="/tmp/rla_b"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_readlinkat")))
long __imported_wali_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
long wali_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    return (long)__imported_wali_readlinkat(dirfd, pathname, buf, bufsiz);
}
#else
#include <sys/syscall.h>
long wali_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    return syscall(SYS_readlinkat, dirfd, pathname, buf, bufsiz);
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

    char buf[64];
    long r = wali_readlinkat(AT_FDCWD, path, buf, sizeof(buf));
    if (!strcmp(mode, "missing"))   return (r < 0) ? 0 : -1;
    if (!strcmp(mode, "not_link"))  return (r < 0) ? 0 : -1;
    if (!strcmp(mode, "ok")) {
        if (r < 0 || r >= (long)sizeof(buf)) return -1;
        buf[r] = '\0';
        return (strcmp(buf, target) == 0) ? 0 : -1;
    }
    return -1;
}
