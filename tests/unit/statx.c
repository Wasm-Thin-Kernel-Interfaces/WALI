// CMD: setup="/tmp/statx_a"  args="ok      /tmp/statx_a"   cleanup="/tmp/statx_a"
// CMD:                        args="missing /tmp/statx_no"  cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#ifndef STATX_ALL
#define STATX_ALL 0x00000fffU
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_statx")))
long __imported_wali_statx(int dirfd, const char *pathname, int flags, unsigned int mask, void *statxbuf);
int wali_statx(int d, const char *p, int f, unsigned int m, void *b) {
    return (int)__imported_wali_statx(d, p, f, m, b);
}
#else
#include <sys/syscall.h>
#ifndef SYS_statx
#define SYS_statx 332
#endif
int wali_statx(int d, const char *p, int f, unsigned int m, void *b) {
    return syscall(SYS_statx, d, p, f, m, b);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    char buf[256];
    memset(buf, 0, sizeof(buf));
    long r = wali_statx(AT_FDCWD, path, 0, STATX_ALL, buf);
    if (!strcmp(mode, "missing")) return (r < 0) ? 0 : -1;
    if (!strcmp(mode, "ok"))      return (r == 0) ? 0 : -1;
    return -1;
}
