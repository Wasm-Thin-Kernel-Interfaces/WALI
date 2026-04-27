// CMD: args="self"
// CMD: args="children"
// CMD: args="bad_who"

#include "wali_start.c"
#include <sys/resource.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getrusage")))
long __imported_wali_getrusage(int who, struct rusage *usage);
int wali_getrusage(int who, struct rusage *usage) { return (int)__imported_wali_getrusage(who, usage); }
#else
#include <sys/syscall.h>
int wali_getrusage(int who, struct rusage *usage) { return syscall(SYS_getrusage, who, usage); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self";

    int who;
    int expect_ok = 1;
    if (!strcmp(mode, "self"))           who = RUSAGE_SELF;
    else if (!strcmp(mode, "children"))  who = RUSAGE_CHILDREN;
    else if (!strcmp(mode, "bad_who"))   { who = 9999; expect_ok = 0; }
    else return -1;

    struct rusage usage;
    memset(&usage, 0, sizeof(usage));
    long r = wali_getrusage(who, &usage);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
