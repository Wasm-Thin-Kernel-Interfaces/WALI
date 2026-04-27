// CMD: args="basic"
// CMD: args="totalram_positive"
// CMD: args="mem_unit_set"

#include "wali_start.c"
#include <sys/sysinfo.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_sysinfo")))
long __imported_wali_sysinfo(struct sysinfo *info);
int wali_sysinfo(struct sysinfo *i) { return (int)__imported_wali_sysinfo(i); }
#else
#include <sys/syscall.h>
int wali_sysinfo(struct sysinfo *i) { return syscall(SYS_sysinfo, i); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "basic";

    struct sysinfo info;
    if (wali_sysinfo(&info) != 0) return -1;

    if (!strcmp(mode, "basic"))             return 0;
    if (!strcmp(mode, "totalram_positive")) return (info.totalram > 0) ? 0 : -1;
    if (!strcmp(mode, "mem_unit_set"))      return (info.mem_unit > 0) ? 0 : -1;
    return -1;
}
