// CMD: args="."
// CMD: args="/tmp/chroot_nonexistent_dir"
// CMD: setup="/tmp/chroot_file.txt"  args="/tmp/chroot_file.txt"  cleanup="/tmp/chroot_file.txt"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
WALI_IMPORT("SYS_chroot") long wali_syscall_chroot(const char *path);
#else
#include <sys/syscall.h>
long wali_syscall_chroot(const char *path) { return syscall(SYS_chroot, path); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *path = (argc > 1) ? argv[1] : ".";
    long r = wali_syscall_chroot(path);
    // Outcome depends on capabilities (CAP_SYS_CHROOT) and path validity;
    // differential check catches WALI/native divergence regardless.
    return (r == 0) ? 0 : 1;
}
