// CMD: args="basic"
// CMD: args="sysname_nonempty"
// CMD: args="release_nonempty"

#include "wali_start.c"
#include <sys/utsname.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_uname")))
long __imported_wali_uname(struct utsname *buf);
int wali_uname(struct utsname *b) { return (int)__imported_wali_uname(b); }
#else
#include <sys/syscall.h>
int wali_uname(struct utsname *b) { return syscall(SYS_uname, b); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    struct utsname u;
    if (wali_uname(&u) != 0) return -1;
    if (!strcmp(mode, "basic"))            return 0;
    if (!strcmp(mode, "sysname_nonempty")) return (strlen(u.sysname) > 0) ? 0 : -1;
    if (!strcmp(mode, "release_nonempty")) return (strlen(u.release) > 0) ? 0 : -1;
    return -1;
}
