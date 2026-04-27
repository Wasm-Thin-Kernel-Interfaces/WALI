// CMD: setup="/tmp/msync_a 4096" args="ms_sync       /tmp/msync_a"  cleanup="/tmp/msync_a"
// CMD: setup="/tmp/msync_b 4096" args="ms_async      /tmp/msync_b"  cleanup="/tmp/msync_b"
// CMD: setup="/tmp/msync_c 4096" args="ms_invalidate /tmp/msync_c"  cleanup="/tmp/msync_c"
// CMD: setup="/tmp/msync_d 4096" args="bad_flags     /tmp/msync_d"  cleanup="/tmp/msync_d"
// CMD:                            args="bad_addr      /tmp/none"      cleanup=""

#define _GNU_SOURCE
#include "wali_start.c"
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_mmap")))
long long __imported_wali_mmap(void *addr, size_t length, int prot, int flags, int fd, long long offset);
__attribute__((__import_module__("wali"), __import_name__("SYS_munmap")))
long long __imported_wali_munmap(void *addr, size_t length);
__attribute__((__import_module__("wali"), __import_name__("SYS_msync")))
long long __imported_wali_msync(void *addr, size_t length, int flags);

void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) {
    long long r = __imported_wali_mmap(a, l, p, f, fd, (long long)o);
    if (r < 0 && r > -4096) return MAP_FAILED;
    return (void *)(long)r;
}
int wali_munmap(void *a, size_t l) { return (int)__imported_wali_munmap(a, l); }
int wali_msync(void *a, size_t l, int flags) { return (int)__imported_wali_msync(a, l, flags); }
#else
#include <sys/syscall.h>
void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) { return mmap(a, l, p, f, fd, o); }
int wali_munmap(void *a, size_t l) { return munmap(a, l); }
int wali_msync(void *a, size_t l, int flags) { return msync(a, l, flags); }
#endif

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 2) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    ftruncate(fd, atoi(argv[1]));
    close(fd);
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
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_addr")) {
        long r = wali_msync((void *)0x100000000ULL, 4096, MS_SYNC);
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDWR, 0);
    if (fd < 0) return -1;
    void *p = wali_mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    wali_syscall_close(fd);
    if (p == MAP_FAILED) return -1;

    int flags;
    int expect_ok = 1;
    if (!strcmp(mode, "ms_sync"))            flags = MS_SYNC;
    else if (!strcmp(mode, "ms_async"))      flags = MS_ASYNC;
    else if (!strcmp(mode, "ms_invalidate")) flags = MS_SYNC | MS_INVALIDATE;
    else if (!strcmp(mode, "bad_flags"))     { flags = 0xDEAD; expect_ok = 0; }
    else { wali_munmap(p, 4096); return -1; }

    long r = wali_msync(p, 4096, flags);
    wali_munmap(p, 4096);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
