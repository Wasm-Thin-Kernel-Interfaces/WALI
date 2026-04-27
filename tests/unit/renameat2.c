// CMD: setup="src:/tmp/r2_a"   args="noflags  /tmp/r2_a /tmp/r2_a_dst" cleanup="/tmp/r2_a /tmp/r2_a_dst"
// CMD: setup="both:/tmp/r2_b /tmp/r2_b_dst" args="exchange /tmp/r2_b /tmp/r2_b_dst" cleanup="/tmp/r2_b /tmp/r2_b_dst"
// CMD: setup="both:/tmp/r2_c /tmp/r2_c_dst" args="noreplace /tmp/r2_c /tmp/r2_c_dst" cleanup="/tmp/r2_c /tmp/r2_c_dst"
// CMD:                          args="src_missing /tmp/r2_no /tmp/r2_dst" cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE (1 << 0)
#endif
#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE  (1 << 1)
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_renameat2")))
long __imported_wali_renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags);
int wali_renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags) {
    return (int)__imported_wali_renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
}
#else
#include <sys/syscall.h>
int wali_renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags) {
    return syscall(SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "src:", 4)) {
        int fd = open(argv[0] + 4, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
    } else if (!strncmp(argv[0], "both:", 5)) {
        if (argc < 2) return -1;
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
        fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
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
    const char *src = argv[2];
    const char *dst = argv[3];

    unsigned int flags = 0;
    int expect_ok = 1;
    if (!strcmp(mode, "noflags"))         flags = 0;
    else if (!strcmp(mode, "exchange"))   flags = RENAME_EXCHANGE;
    else if (!strcmp(mode, "noreplace"))  { flags = RENAME_NOREPLACE; expect_ok = 0; }  // dst exists → EEXIST
    else if (!strcmp(mode, "src_missing")) { flags = 0; expect_ok = 0; }
    else return -1;

    long r = wali_renameat2(AT_FDCWD, src, AT_FDCWD, dst, flags);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
