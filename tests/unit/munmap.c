// CMD: args="ok"
// CMD: args="size_zero"
// CMD: args="unaligned"

#include "wali_start.c"
#include <sys/mman.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_mmap")))
long long __imported_wali_mmap(void *addr, size_t length, int prot, int flags, int fd, long long offset);
__attribute__((__import_module__("wali"), __import_name__("SYS_munmap")))
long long __imported_wali_munmap(void *addr, size_t length);
void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) {
    long long r = __imported_wali_mmap(a, l, p, f, fd, (long long)o);
    if (r < 0 && r > -4096) return MAP_FAILED;
    return (void *)(long)r;
}
int wali_munmap(void *a, size_t l) { return (int)__imported_wali_munmap(a, l); }
#else
#include <sys/syscall.h>
void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) { return (void *)syscall(SYS_mmap, a, l, p, f, fd, o); }
int wali_munmap(void *a, size_t l) { return syscall(SYS_munmap, a, l); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ok";

    void *ptr = wali_mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return -1;

    long r;
    int expect_ok = 1;
    if (!strcmp(mode, "ok")) {
        r = wali_munmap(ptr, 4096);
    } else if (!strcmp(mode, "size_zero")) {
        // Linux: munmap(addr, 0) returns -1 with EINVAL.
        r = wali_munmap(ptr, 0);
        expect_ok = 0;
        wali_munmap(ptr, 4096);
    } else if (!strcmp(mode, "unaligned")) {
        // munmap with unaligned addr → EINVAL.
        r = wali_munmap((char *)ptr + 1, 4096);
        expect_ok = 0;
        wali_munmap(ptr, 4096);
    } else {
        wali_munmap(ptr, 4096);
        return -1;
    }
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
