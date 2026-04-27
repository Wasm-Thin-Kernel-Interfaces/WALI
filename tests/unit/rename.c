// CMD: setup="src:/tmp/ren_a"  args="ok          /tmp/ren_a /tmp/ren_a_dst"   cleanup="/tmp/ren_a /tmp/ren_a_dst"
// CMD:                          args="src_missing /tmp/ren_no /tmp/ren_dst"    cleanup=""
// CMD: setup="both:/tmp/ren_b /tmp/ren_b_dst" args="overwrite /tmp/ren_b /tmp/ren_b_dst" cleanup="/tmp/ren_b /tmp/ren_b_dst"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_rename")))
long __imported_wali_rename(const char *oldpath, const char *newpath);
int wali_rename(const char *oldpath, const char *newpath) { return (int)__imported_wali_rename(oldpath, newpath); }
#else
#include <sys/syscall.h>
int wali_rename(const char *oldpath, const char *newpath) {
#ifdef SYS_rename
    return syscall(SYS_rename, oldpath, newpath);
#else
    return syscall(SYS_renameat, AT_FDCWD, oldpath, AT_FDCWD, newpath);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "src:", 4)) {
        int fd = open(argv[0] + 4, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { write(fd, "DATA", 4); close(fd); }
    } else if (!strncmp(argv[0], "both:", 5)) {
        if (argc < 2) return -1;
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { write(fd, "SRC", 3); close(fd); }
        fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { write(fd, "DST", 3); close(fd); }
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

    int expect_ok = strcmp(mode, "src_missing") != 0;
    long r = wali_rename(src, dst);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
