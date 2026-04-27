// CMD: setup="file:/tmp/lstat_a"        args="reg /tmp/lstat_a"      cleanup="/tmp/lstat_a"
// CMD: setup="symlink:/tmp/lstat_b /tmp/lstat_b_target" args="symlink /tmp/lstat_b" cleanup="/tmp/lstat_b /tmp/lstat_b_target"
// CMD:                                    args="missing /tmp/lstat_no" cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_lstat")))
long __imported_wali_lstat(const char *pathname, struct stat *statbuf);
int wali_lstat(const char *pathname, struct stat *statbuf) { return (int)__imported_wali_lstat(pathname, statbuf); }
#else
#include <sys/syscall.h>
int wali_lstat(const char *pathname, struct stat *statbuf) {
#ifdef SYS_lstat
    return syscall(SYS_lstat, pathname, statbuf);
#else
    return syscall(SYS_newfstatat, AT_FDCWD, pathname, statbuf, AT_SYMLINK_NOFOLLOW);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "file:", 5)) {
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
    } else if (!strncmp(argv[0], "symlink:", 8)) {
        if (argc < 2) return -1;
        // symlink:<linkpath> <targetpath>
        const char *linkpath = argv[0] + 8;
        const char *target = argv[1];
        // Create the target file too so the symlink isn't dangling.
        int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
        unlink(linkpath);
        if (symlink(target, linkpath) != 0) return -1;
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
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    struct stat st;
    long r = wali_lstat(path, &st);

    if (!strcmp(mode, "missing")) return (r < 0) ? 0 : -1;
    if (r != 0) return -1;
    if (!strcmp(mode, "reg"))     return S_ISREG(st.st_mode) ? 0 : -1;
    if (!strcmp(mode, "symlink")) return S_ISLNK(st.st_mode) ? 0 : -1;
    return -1;
}
