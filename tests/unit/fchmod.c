// CMD: setup="/tmp/fchmod_a"  args="0777 /tmp/fchmod_a"  cleanup="0777 /tmp/fchmod_a"
// CMD: setup="/tmp/fchmod_b"  args="0600 /tmp/fchmod_b"  cleanup="0600 /tmp/fchmod_b"
// CMD: setup="/tmp/fchmod_c"  args="0000 /tmp/fchmod_c"  cleanup="0000 /tmp/fchmod_c"
// CMD: setup="/tmp/fchmod_d"  args="0644 /tmp/fchmod_d"  cleanup="0644 /tmp/fchmod_d"
// CMD:                         args="bad_fd /tmp/none"     cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) { unlink(argv[0]); return 0; }
    long expected = strtol(argv[0], NULL, 8);
    const char *path = argv[1];
    struct stat st;
    int ok = (stat(path, &st) == 0) && ((st.st_mode & 07777) == (mode_t)expected);
    unlink(path);
    return ok ? 0 : -1;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode_str = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode_str, "bad_fd")) {
        long r = wali_syscall_fchmod(99999, 0644);
        return (r < 0) ? 0 : -1;
    }

    int mode = 0;
    for (const char *p = mode_str; *p; p++) {
        if (*p < '0' || *p > '7') return -1;
        mode = (mode << 3) | (*p - '0');
    }

    int fd = wali_syscall_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    long r = wali_syscall_fchmod(fd, mode);
    wali_syscall_close(fd);
    return (r == 0) ? 0 : -1;
}
