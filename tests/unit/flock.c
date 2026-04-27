// CMD: setup="/tmp/flock_a" args="ex_un /tmp/flock_a" cleanup="/tmp/flock_a"
// CMD: setup="/tmp/flock_b" args="sh_un /tmp/flock_b" cleanup="/tmp/flock_b"
// CMD: setup="/tmp/flock_c" args="ex_to_sh /tmp/flock_c" cleanup="/tmp/flock_c"
// CMD:                       args="bad_fd /tmp/none"     cleanup=""
// CMD: setup="/tmp/flock_d" args="bad_op /tmp/flock_d" cleanup="/tmp/flock_d"

#include "wali_start.c"
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_flock") long wali_syscall_flock(int fd, int operation);
int wali_flock(int fd, int op) { return (int)wali_syscall_flock(fd, op); }
#else
#include <sys/syscall.h>
int wali_flock(int fd, int op) { return syscall(SYS_flock, fd, op); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
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
        long r = wali_flock(99999, LOCK_EX);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDWR, 0);
    if (fd < 0) return -1;

    int ret = -1;
    if (!strcmp(mode, "ex_un")) {
        if (wali_flock(fd, LOCK_EX) == 0 && wali_flock(fd, LOCK_UN) == 0) ret = 0;
    } else if (!strcmp(mode, "sh_un")) {
        if (wali_flock(fd, LOCK_SH) == 0 && wali_flock(fd, LOCK_UN) == 0) ret = 0;
    } else if (!strcmp(mode, "ex_to_sh")) {
        // Lock upgrade/downgrade: EX → SH should succeed (atomic conversion).
        if (wali_flock(fd, LOCK_EX) == 0 && wali_flock(fd, LOCK_SH) == 0) ret = 0;
    } else if (!strcmp(mode, "bad_op")) {
        long r = wali_flock(fd, 0xDEAD);
        ret = (r < 0) ? 0 : -1;
    }

    wali_syscall_close(fd);
    return ret;
}
