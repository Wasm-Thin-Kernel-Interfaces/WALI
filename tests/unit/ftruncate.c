// CMD: setup="/tmp/ft_a" args="2  /tmp/ft_a"  cleanup="2 /tmp/ft_a"
// CMD: setup="/tmp/ft_b" args="10 /tmp/ft_b"  cleanup="10 /tmp/ft_b"
// CMD: setup="/tmp/ft_c" args="0  /tmp/ft_c"  cleanup="0 /tmp/ft_c"
// CMD:                    args="bad_fd /tmp/none"   cleanup=""
// CMD: setup="/tmp/ft_d" args="negative /tmp/ft_d"  cleanup="/tmp/ft_d"

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_ftruncate") long wali_syscall_ftruncate(int fd, long long length);
int wali_ftruncate(int fd, long long length) { return (int)wali_syscall_ftruncate(fd, length); }
#else
#include <sys/syscall.h>
int wali_ftruncate(int fd, long long length) { return syscall(SYS_ftruncate, fd, length); }
#endif

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, "12345", 5);
    close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) { unlink(argv[0]); return 0; }
    long expected = atol(argv[0]);
    const char *path = argv[1];
    struct stat st;
    int ok = (stat(path, &st) == 0) && (st.st_size == expected);
    unlink(path);
    return ok ? 0 : -1;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *length_str = argv[1];
    const char *path = argv[2];

    if (!strcmp(length_str, "bad_fd")) {
        long r = wali_ftruncate(99999, 0);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(length_str, "negative")) {
        int fd = wali_syscall_open(path, O_RDWR, 0);
        if (fd < 0) return -1;
        long r = wali_ftruncate(fd, -1);
        wali_syscall_close(fd);
        return (r < 0) ? 0 : -1;
    }

    long long length = 0;
    for (const char *p = length_str; *p; p++) {
        if (*p < '0' || *p > '9') return -1;
        length = length * 10 + (*p - '0');
    }

    int fd = wali_syscall_open(path, O_RDWR, 0);
    if (fd < 0) return -1;
    long r = wali_ftruncate(fd, length);
    wali_syscall_close(fd);
    return (r == 0) ? 0 : -1;
}
