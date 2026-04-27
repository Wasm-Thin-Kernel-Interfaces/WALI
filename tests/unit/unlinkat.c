// CMD: setup="dir_with_file:/tmp/ula_a"     args="file_relative /tmp/ula_a" cleanup="dir_with_file:/tmp/ula_a"
// CMD: setup="dir_with_subdir:/tmp/ula_b"   args="dir_relative  /tmp/ula_b" cleanup="dir_with_subdir:/tmp/ula_b"
// CMD: setup="dir:/tmp/ula_c"               args="missing       /tmp/ula_c" cleanup="dir:/tmp/ula_c"

#define _GNU_SOURCE
#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_unlinkat")))
long __imported_wali_unlinkat(int dirfd, const char *pathname, int flags);
int wali_unlinkat(int d, const char *p, int f) { return (int)__imported_wali_unlinkat(d, p, f); }
#else
#include <sys/syscall.h>
int wali_unlinkat(int d, const char *p, int f) { return syscall(SYS_unlinkat, d, p, f); }
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
    if (!strncmp(argv[0], "dir_with_file:", 14)) {
        const char *dir = argv[0] + 14;
        char buf[256];
        snprintf(buf, sizeof(buf), "%s/f1", dir);
        unlink(buf); rmdir(dir);
        if (mkdir(dir, 0755) != 0) return -1;
        int fd = open(buf, O_WRONLY | O_CREAT, 0644);
        if (fd >= 0) close(fd);
    }
    if (!strncmp(argv[0], "dir_with_subdir:", 16)) {
        const char *dir = argv[0] + 16;
        char buf[256];
        snprintf(buf, sizeof(buf), "%s/sub", dir);
        rmdir(buf); rmdir(dir);
        if (mkdir(dir, 0755) != 0) return -1;
        return mkdir(buf, 0755);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "dir_with_file:", 14)) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/f1", argv[i] + 14);
            unlink(buf); rmdir(argv[i] + 14);
        } else if (!strncmp(argv[i], "dir_with_subdir:", 16)) {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s/sub", argv[i] + 16);
            rmdir(buf); rmdir(argv[i] + 16);
        } else if (!strncmp(argv[i], "dir:", 4)) {
            rmdir(argv[i] + 4);
        } else {
            unlink(argv[i]);
        }
    }
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *dir = argv[2];

    int dirfd = wali_syscall_open(dir, O_RDONLY | O_DIRECTORY, 0);
    if (dirfd < 0) return -1;

    long r;
    int expect_ok = 1;
    if (!strcmp(mode, "file_relative")) {
        r = wali_unlinkat(dirfd, "f1", 0);
    } else if (!strcmp(mode, "dir_relative")) {
        r = wali_unlinkat(dirfd, "sub", AT_REMOVEDIR);
    } else if (!strcmp(mode, "missing")) {
        r = wali_unlinkat(dirfd, "no_such_file", 0);
        expect_ok = 0;
    } else {
        wali_syscall_close(dirfd);
        return -1;
    }

    wali_syscall_close(dirfd);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
