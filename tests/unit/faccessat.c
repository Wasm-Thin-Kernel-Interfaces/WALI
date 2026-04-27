// CMD: setup="0644 /tmp/faccessat_a"  args="F_OK 1 /tmp/faccessat_a"     cleanup="/tmp/faccessat_a"
// CMD:                                  args="F_OK 0 /tmp/faccessat_miss" cleanup=""
// CMD: setup="0444 /tmp/faccessat_b"  args="R_OK 1 /tmp/faccessat_b"     cleanup="/tmp/faccessat_b"
// CMD: setup="0755 /tmp/faccessat_c"  args="X_OK 1 /tmp/faccessat_c"     cleanup="/tmp/faccessat_c"
// CMD: setup="0644 /tmp/faccessat_d"  args="X_OK 0 /tmp/faccessat_d"     cleanup="/tmp/faccessat_d"

#include "wali_start.c"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_faccessat")))
long __imported_wali_faccessat(int dirfd, const char *pathname, int mode, int flags);
int wali_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    return (int)__imported_wali_faccessat(dirfd, pathname, mode, flags);
}
#else
#include <sys/syscall.h>
int wali_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    return syscall(SYS_faccessat, dirfd, pathname, mode, flags);
}
#endif

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 2) return 0;
    long mode = strtol(argv[0], NULL, 8);
    const char *path = argv[1];
    unlink(path);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)mode);
    if (fd < 0) return -1;
    close(fd);
    return chmod(path, (mode_t)mode);
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
#endif

static int parse_mode(const char *s) {
    if (!strcmp(s, "F_OK")) return F_OK;
    if (!strcmp(s, "R_OK")) return R_OK;
    if (!strcmp(s, "W_OK")) return W_OK;
    if (!strcmp(s, "X_OK")) return X_OK;
    return -1;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 4) return -1;
    int mode = parse_mode(argv[1]);
    if (mode < 0) return -1;
    int expect_ok = (argv[2][0] == '1');
    const char *path = argv[3];

    long r = wali_faccessat(AT_FDCWD, path, mode, 0);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
