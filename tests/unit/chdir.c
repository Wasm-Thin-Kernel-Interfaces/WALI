// CMD: setup="mkdir /tmp/sub_chdir"  args="ok /tmp/sub_chdir"            cleanup="rmdir /tmp/sub_chdir"
// CMD:                                args="ok /tmp"                      cleanup=""
// CMD:                                args="ok .."                        cleanup=""
// CMD:                                args="fail /tmp/chdir_nonexistent"  cleanup=""
// CMD:                                args="fail ''"                      cleanup=""

#include "wali_start.c"
#include <string.h>

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
int test_setup(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[0], "mkdir") == 0) {
        rmdir(argv[1]);
        return mkdir(argv[1], 0755);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[0], "rmdir") == 0) {
        rmdir(argv[1]);
    }
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    int expect_ok = strcmp(argv[1], "ok") == 0;
    long r = wali_syscall_chdir(argv[2]);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
