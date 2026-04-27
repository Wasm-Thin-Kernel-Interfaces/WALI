// CMD:                            args="ok         /tmp/sla_target  /tmp/sla_link"  cleanup="/tmp/sla_link"
// CMD: setup="exists:/tmp/sla_b"  args="dst_exists /tmp/sla_target  /tmp/sla_b"     cleanup="/tmp/sla_b"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_symlinkat")))
long __imported_wali_symlinkat(const char *target, int newdirfd, const char *linkpath);
int wali_symlinkat(const char *t, int d, const char *l) { return (int)__imported_wali_symlinkat(t, d, l); }
#else
#include <sys/syscall.h>
int wali_symlinkat(const char *t, int d, const char *l) { return syscall(SYS_symlinkat, t, d, l); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "exists:", 7)) {
        int fd = open(argv[0] + 7, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    for (int i = 0; i < argc; i++) unlink(argv[i]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 4) return -1;
    const char *mode = argv[1];
    const char *target = argv[2];
    const char *link = argv[3];

    int expect_ok = !strcmp(mode, "ok");
    long r = wali_symlinkat(target, AT_FDCWD, link);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
