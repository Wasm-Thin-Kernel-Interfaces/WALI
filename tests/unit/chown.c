// CMD: setup="/tmp/chown_a.txt"  args="/tmp/chown_a.txt   -1 -1"  cleanup="/tmp/chown_a.txt"
// CMD:                             args="/tmp/chown_missing -1 -1"  cleanup=""
// CMD: setup="/tmp/chown_b.txt"  args="/tmp/chown_b.txt   -1  0"  cleanup="/tmp/chown_b.txt"
// CMD: setup="/tmp/chown_c.txt"  args="/tmp/chown_c.txt    0  0"  cleanup="/tmp/chown_c.txt"
// CMD: setup="/tmp/chown_d.txt"  args="/tmp/chown_d.txt    0 -1"  cleanup="/tmp/chown_d.txt"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

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

static int parse_int(const char *s) {
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return sign * v;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 4) return -1;
    long r = wali_syscall_chown(argv[1], parse_int(argv[2]), parse_int(argv[3]));
    // Return chown's result directly; the differential check catches divergence
    // between native and WASM regardless of root/non-root context.
    return (r == 0) ? 0 : 1;
}
