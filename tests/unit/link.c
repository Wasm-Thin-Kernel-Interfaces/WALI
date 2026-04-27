// CMD: setup="src:/tmp/link_a"  args="ok          /tmp/link_a /tmp/link_a_dst"  cleanup="/tmp/link_a /tmp/link_a_dst"
// CMD:                            args="src_missing /tmp/link_miss /tmp/link_dst" cleanup=""
// CMD: setup="both:/tmp/link_b /tmp/link_b_dst" args="dst_exists /tmp/link_b /tmp/link_b_dst" cleanup="/tmp/link_b /tmp/link_b_dst"
// CMD: setup="src:/tmp/link_c"  args="bad_dst    /tmp/link_c /nonexistent_dir/foo" cleanup="/tmp/link_c"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_link")))
long __imported_wali_link(const char *oldpath, const char *newpath);
int wali_link(const char *oldpath, const char *newpath) { return (int)__imported_wali_link(oldpath, newpath); }
#else
#include <sys/syscall.h>
int wali_link(const char *oldpath, const char *newpath) {
#ifdef SYS_link
    return syscall(SYS_link, oldpath, newpath);
#else
    return syscall(SYS_linkat, AT_FDCWD, oldpath, AT_FDCWD, newpath, 0);
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
        // setup creates both src and dst (so dst exists for the EEXIST test).
        if (argc < 2) return -1;
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { write(fd, "DATA", 4); close(fd); }
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
    long r = wali_link(src, dst);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
