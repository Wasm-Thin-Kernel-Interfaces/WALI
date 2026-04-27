// CMD: setup="dir:/tmp/rmd_a"            args="ok          /tmp/rmd_a"   cleanup="/tmp/rmd_a"
// CMD:                                     args="missing     /tmp/rmd_no"  cleanup=""
// CMD: setup="dir_with_file:/tmp/rmd_b"   args="not_empty   /tmp/rmd_b"   cleanup="dir_with_file:/tmp/rmd_b"
// CMD: setup="file:/tmp/rmd_c"            args="not_a_dir   /tmp/rmd_c"   cleanup="/tmp/rmd_c"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_rmdir")))
long __imported_wali_rmdir(const char *pathname);
int wali_rmdir(const char *pathname) { return (int)__imported_wali_rmdir(pathname); }
#else
int wali_rmdir(const char *pathname) { return wali_syscall_rmdir(pathname); }
#endif

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
#include <stdio.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "dir:", 4)) {
        rmdir(argv[0] + 4);
        return mkdir(argv[0] + 4, 0755);
    }
    if (!strncmp(argv[0], "file:", 5)) {
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
    }
    if (!strncmp(argv[0], "dir_with_file:", 14)) {
        const char *dir = argv[0] + 14;
        char buf[256];
        snprintf(buf, sizeof(buf), "%s/f1", dir);
        unlink(buf);
        rmdir(dir);
        if (mkdir(dir, 0755) != 0) return -1;
        int fd = open(buf, O_WRONLY | O_CREAT, 0644);
        if (fd >= 0) close(fd);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "dir_with_file:", 14)) {
            const char *dir = argv[i] + 14;
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/f1", dir);
            unlink(buf);
            rmdir(dir);
        } else {
            unlink(argv[i]);
            rmdir(argv[i]);
        }
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
    long r = wali_rmdir(path);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
