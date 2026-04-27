// CMD: setup="clean /tmp/mkdir_a"   args="ok      /tmp/mkdir_a"   cleanup="/tmp/mkdir_a"
// CMD: setup="create /tmp/mkdir_b"  args="exists  /tmp/mkdir_b"  cleanup="/tmp/mkdir_b"
// CMD:                                args="no_parent /tmp/mkdir_p/c" cleanup=""
// CMD: setup="clean /tmp/mkdir_d"   args="mode    /tmp/mkdir_d"   cleanup="0700 /tmp/mkdir_d"

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 2) return 0;
    if (!strcmp(argv[0], "create"))     mkdir(argv[1], 0755);
    else if (!strcmp(argv[0], "clean")) rmdir(argv[1]);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) { rmdir(argv[0]); return 0; }
    // 2 args: expected_mode path — verify the dir we made has the right mode.
    long expected = strtol(argv[0], NULL, 8);
    const char *path = argv[1];
    struct stat st;
    int ok = (stat(path, &st) == 0) && S_ISDIR(st.st_mode) && ((st.st_mode & 0777) == (mode_t)expected);
    rmdir(path);
    return ok ? 0 : -1;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    int permission = !strcmp(mode, "mode") ? 0700 : 0755;
    int expect_ok = !strcmp(mode, "ok") || !strcmp(mode, "mode");

    long r = wali_syscall_mkdir(path, permission);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
