// CMD: setup="src:/tmp/linkat_a"  args="ok          /tmp/linkat_a /tmp/linkat_a_dst"  cleanup="/tmp/linkat_a /tmp/linkat_a_dst"
// CMD:                              args="src_missing /tmp/linkat_miss /tmp/linkat_dst" cleanup=""
// CMD: setup="both:/tmp/linkat_b /tmp/linkat_b_dst" args="dst_exists /tmp/linkat_b /tmp/linkat_b_dst" cleanup="/tmp/linkat_b /tmp/linkat_b_dst"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_linkat")))
long __imported_wali_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
int wali_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    return (int)__imported_wali_linkat(olddirfd, oldpath, newdirfd, newpath, flags);
}
#else
#include <sys/syscall.h>
int wali_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    return syscall(SYS_linkat, olddirfd, oldpath, newdirfd, newpath, flags);
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

    int expect_ok = !strcmp(mode, "ok");
    long r = wali_linkat(AT_FDCWD, src, AT_FDCWD, dst, 0);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
