// CMD: args="cwd ."
// CMD: args="root /"
// CMD: args="tmp /tmp"
// CMD: args="bad_fd ."

#include "wali_start.c"
#include <fcntl.h>
#include <sys/statfs.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_fstatfs")))
long __imported_wali_fstatfs(int fd, struct statfs *buf);
int wali_fstatfs(int fd, struct statfs *buf) { return (int)__imported_wali_fstatfs(fd, buf); }
#else
#include <sys/syscall.h>
int wali_fstatfs(int fd, struct statfs *buf) { return syscall(SYS_fstatfs, fd, buf); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_fd")) {
        struct statfs st;
        long r = wali_fstatfs(99999, &st);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    struct statfs st;
    long r = wali_fstatfs(fd, &st);
    wali_syscall_close(fd);
    if (r != 0) return -1;
    return (st.f_type != 0) ? 0 : -1;
}
