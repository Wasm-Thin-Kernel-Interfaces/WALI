// CMD: setup="file:/tmp/newfstatat_a"        args="file        /tmp/newfstatat_a"  cleanup="/tmp/newfstatat_a"
// CMD: setup="dir:/tmp/newfstatat_d"          args="dir         /tmp/newfstatat_d"  cleanup="dir:/tmp/newfstatat_d"
// CMD: setup="symlink:/tmp/newfstatat_s /tmp/newfstatat_t" args="symlink_nofollow /tmp/newfstatat_s" cleanup="/tmp/newfstatat_s /tmp/newfstatat_t"
// CMD:                                         args="missing     /tmp/newfstatat_no"  cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_newfstatat")))
long __imported_wali_newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags);
int wali_newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
    return (int)__imported_wali_newfstatat(dirfd, pathname, statbuf, flags);
}
#else
#include <sys/syscall.h>
int wali_newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
    return syscall(SYS_newfstatat, dirfd, pathname, statbuf, flags);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "file:", 5)) {
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { write(fd, "DATA", 4); close(fd); }
    } else if (!strncmp(argv[0], "dir:", 4)) {
        rmdir(argv[0] + 4);
        return mkdir(argv[0] + 4, 0755);
    } else if (!strncmp(argv[0], "symlink:", 8)) {
        if (argc < 2) return -1;
        const char *link = argv[0] + 8;
        const char *target = argv[1];
        int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
        unlink(link);
        return symlink(target, link);
    }
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

    struct stat st;
    int flags = 0;
    if (!strcmp(mode, "symlink_nofollow")) flags = AT_SYMLINK_NOFOLLOW;

    long r = wali_newfstatat(AT_FDCWD, path, &st, flags);

    if (!strcmp(mode, "missing")) return (r < 0) ? 0 : -1;
    if (r != 0) return -1;
    if (!strcmp(mode, "file"))             return S_ISREG(st.st_mode) ? 0 : -1;
    if (!strcmp(mode, "dir"))              return S_ISDIR(st.st_mode) ? 0 : -1;
    if (!strcmp(mode, "symlink_nofollow")) return S_ISLNK(st.st_mode) ? 0 : -1;
    return -1;
}
