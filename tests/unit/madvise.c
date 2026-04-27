// CMD: args="normal"
// CMD: args="random"
// CMD: args="sequential"
// CMD: args="willneed"
// CMD: args="dontneed"
// CMD: args="bad_advice"

#define _GNU_SOURCE
#include "wali_start.c"
#include <sys/mman.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_mmap")))
long long __imported_wali_mmap(void *addr, size_t length, int prot, int flags, int fd, long long offset);
__attribute__((__import_module__("wali"), __import_name__("SYS_munmap")))
long long __imported_wali_munmap(void *addr, size_t length);
__attribute__((__import_module__("wali"), __import_name__("SYS_madvise")))
long long __imported_wali_madvise(void *addr, size_t length, int advice);

void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) {
    long long r = __imported_wali_mmap(a, l, p, f, fd, (long long)o);
    if (r < 0 && r > -4096) return MAP_FAILED;
    return (void *)(long)r;
}
int wali_munmap(void *a, size_t l) { return (int)__imported_wali_munmap(a, l); }
int wali_madvise(void *a, size_t l, int advice) { return (int)__imported_wali_madvise(a, l, advice); }
#else
int wali_mmap_dummy(void) { return 0; }
void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) { return mmap(a, l, p, f, fd, o); }
int wali_munmap(void *a, size_t l) { return munmap(a, l); }
int wali_madvise(void *a, size_t l, int advice) { return madvise(a, l, advice); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "normal";

    int advice;
    int expect_ok = 1;
    if (!strcmp(mode, "normal"))           advice = MADV_NORMAL;
    else if (!strcmp(mode, "random"))      advice = MADV_RANDOM;
    else if (!strcmp(mode, "sequential"))  advice = MADV_SEQUENTIAL;
    else if (!strcmp(mode, "willneed"))    advice = MADV_WILLNEED;
    else if (!strcmp(mode, "dontneed"))    advice = MADV_DONTNEED;
    else if (!strcmp(mode, "bad_advice"))  { advice = 9999; expect_ok = 0; }
    else return -1;

    size_t size = 4096;
    void *ptr = wali_mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return -1;

    long r = wali_madvise(ptr, size, advice);
    wali_munmap(ptr, size);
    int success = (r == 0);
    return (success == expect_ok) ? 0 : -1;
}
