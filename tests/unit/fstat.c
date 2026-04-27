// CMD: setup="/tmp/fstat_a"   args="file /tmp/fstat_a"  cleanup="/tmp/fstat_a"
// CMD: setup="dir:/tmp/fstat_d" args="dir /tmp/fstat_d" cleanup="dir:/tmp/fstat_d"
// CMD:                          args="bad_fd /tmp/none"  cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif
#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH 0x1000
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_fstat")))
long __imported_wali_fstat(int fd, struct stat *statbuf);
int wali_fstat(int fd, struct stat *statbuf) { return (int)__imported_wali_fstat(fd, statbuf); }
#else
int wali_fstat(int fd, struct stat *statbuf) {
#ifdef SYS_fstat
    return syscall(SYS_fstat, fd, statbuf);
#else
    return syscall(SYS_newfstatat, fd, "", statbuf, AT_EMPTY_PATH);
#endif
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "dir:", 4)) {
        rmdir(argv[0] + 4);
        return mkdir(argv[0] + 4, 0755);
    }
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, "TEST", 4);
    close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "dir:", 4)) { rmdir(argv[0] + 4); return 0; }
    unlink(argv[0]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_fd")) {
        struct stat st;
        long r = wali_fstat(99999, &st);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDONLY | (!strcmp(mode, "dir") ? O_DIRECTORY : 0), 0);
    if (fd < 0) return -1;

    struct stat st;
    long r = wali_fstat(fd, &st);
    wali_syscall_close(fd);
    if (r != 0) return -1;

    if (!strcmp(mode, "file")) {
        if (!S_ISREG(st.st_mode)) return -1;
        if (st.st_size != 4) return -1;
    } else if (!strcmp(mode, "dir")) {
        if (!S_ISDIR(st.st_mode)) return -1;
    }
    return 0;
}
