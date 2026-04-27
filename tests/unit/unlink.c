// CMD: setup="/tmp/unlink_a" args="ok      /tmp/unlink_a"  cleanup="/tmp/unlink_a"
// CMD:                        args="missing /tmp/unlink_no" cleanup=""
// CMD: setup="dir:/tmp/unlink_d" args="dir   /tmp/unlink_d" cleanup="dir:/tmp/unlink_d"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_unlink")))
long __imported_wali_unlink(const char *pathname);
int wali_unlink(const char *p) { return (int)__imported_wali_unlink(p); }
#else
int wali_unlink(const char *p) {
#ifdef SYS_unlink
    return syscall(SYS_unlink, p);
#else
    return syscall(SYS_unlinkat, AT_FDCWD, p, 0);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "dir:", 4)) {
        rmdir(argv[0] + 4);
        return mkdir(argv[0] + 4, 0755);
    }
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "dir:", 4)) rmdir(argv[i] + 4);
        else unlink(argv[i]);
    }
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    int expect_ok = !strcmp(mode, "ok");
    long r = wali_unlink(path);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
