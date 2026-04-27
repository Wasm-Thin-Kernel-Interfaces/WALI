// CMD: setup="/tmp/umask_a" args="044 0666 0622 /tmp/umask_a" cleanup="0622 /tmp/umask_a"
// CMD: setup="/tmp/umask_b" args="022 0666 0644 /tmp/umask_b" cleanup="0644 /tmp/umask_b"
// CMD: setup="/tmp/umask_c" args="077 0666 0600 /tmp/umask_c" cleanup="0600 /tmp/umask_c"

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_umask")))
long __imported_wali_umask(int mask);
int wali_umask(int m) { return (int)__imported_wali_umask(m); }
#else
#include <sys/syscall.h>
int wali_umask(int m) { return syscall(SYS_umask, m); }
#endif

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) { unlink(argv[0]); return 0; }
    long expected = strtol(argv[0], NULL, 8);
    const char *path = argv[1];
    struct stat st;
    int ok = (stat(path, &st) == 0) && ((st.st_mode & 0777) == (mode_t)expected);
    unlink(path);
    return ok ? 0 : -1;
}
#endif

static int parse_octal(const char *s) {
    int v = 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '7') return -1;
        v = (v << 3) | (*s - '0');
    }
    return v;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 5) return -1;
    int mask    = parse_octal(argv[1]);
    int request = parse_octal(argv[2]);
    /* expected   = parse_octal(argv[3]); — verified by cleanup */
    const char *path = argv[4];
    if (mask < 0 || request < 0) return -1;

    wali_umask(mask);
    int fd = wali_syscall_open(path, O_CREAT | O_WRONLY, request);
    if (fd < 0) return -1;
    wali_syscall_close(fd);
    return 0;
}
