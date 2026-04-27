// CMD: setup="dir:/tmp/openat_a"          args="create_relative /tmp/openat_a"   cleanup="rmdirsub:/tmp/openat_a/f1 /tmp/openat_a"
// CMD: setup="dir_with_file:/tmp/openat_b" args="read_existing   /tmp/openat_b"   cleanup="rmdirsub:/tmp/openat_b/f1 /tmp/openat_b"
// CMD: setup="dir:/tmp/openat_c"          args="bad_dirfd       /tmp/openat_c"   cleanup="rmdirsub: /tmp/openat_c"
// CMD: setup="dir:/tmp/openat_d"          args="nonexistent     /tmp/openat_d"   cleanup="rmdirsub: /tmp/openat_d"
// CMD: setup="dir_with_file:/tmp/openat_e" args="excl_existing   /tmp/openat_e"   cleanup="rmdirsub:/tmp/openat_e/f1 /tmp/openat_e"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_openat")))
long __imported_wali_openat(int dirfd, const char *pathname, int flags, int mode);
int wali_openat(int dirfd, const char *pathname, int flags, int mode) { return (int)__imported_wali_openat(dirfd, pathname, flags, mode); }
#else
#include <sys/syscall.h>
int wali_openat(int dirfd, const char *pathname, int flags, int mode) { return syscall(SYS_openat, dirfd, pathname, flags, mode); }
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
        unlink(buf);
        rmdir(dir);
        if (mkdir(dir, 0755) != 0) return -1;
        int fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    // Args are paths to delete; "rmdirsub:" prefix means use rmdir.
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "rmdirsub:", 9)) {
            // Either a file path or empty (just marker).
            if (argv[i][9]) unlink(argv[i] + 9);
        } else {
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
    const char *dir = argv[2];

    int dirfd;
    int expect_ok = 1;
    if (!strcmp(mode, "bad_dirfd")) {
        dirfd = -1;
        expect_ok = 0;
    } else {
        dirfd = wali_syscall_open(dir, O_RDONLY | O_DIRECTORY, 0);
        if (dirfd < 0) return -1;
    }

    long r;
    if (!strcmp(mode, "create_relative")) {
        r = wali_openat(dirfd, "f1", O_WRONLY | O_CREAT, 0644);
    } else if (!strcmp(mode, "read_existing")) {
        r = wali_openat(dirfd, "f1", O_RDONLY, 0);
    } else if (!strcmp(mode, "bad_dirfd")) {
        r = wali_openat(-1, "f1", O_RDONLY, 0);
    } else if (!strcmp(mode, "nonexistent")) {
        r = wali_openat(dirfd, "no_such_file", O_RDONLY, 0);
        expect_ok = 0;
    } else if (!strcmp(mode, "excl_existing")) {
        r = wali_openat(dirfd, "f1", O_WRONLY | O_CREAT | O_EXCL, 0644);
        expect_ok = 0;
    } else {
        if (strcmp(mode, "bad_dirfd") != 0) wali_syscall_close(dirfd);
        return -1;
    }

    int success = (r >= 0);
    if (success) wali_syscall_close((int)r);
    if (strcmp(mode, "bad_dirfd") != 0) wali_syscall_close(dirfd);
    return (success == expect_ok) ? 0 : -1;
}
