// CMD: args="rw_to_ro"
// CMD: args="ro_to_rw"
// CMD: args="bad_addr"

#include "wali_start.c"
#include <sys/mman.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_mmap")))
long long __imported_wali_mmap(void *addr, size_t length, int prot, int flags, int fd, long long offset);
__attribute__((__import_module__("wali"), __import_name__("SYS_munmap")))
long long __imported_wali_munmap(void *addr, size_t length);
__attribute__((__import_module__("wali"), __import_name__("SYS_mprotect")))
long long __imported_wali_mprotect(void *addr, size_t len, int prot);

void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) {
    long long r = __imported_wali_mmap(a, l, p, f, fd, (long long)o);
    if (r < 0 && r > -4096) return MAP_FAILED;
    return (void *)(long)r;
}
int wali_munmap(void *a, size_t l) { return (int)__imported_wali_munmap(a, l); }
int wali_mprotect(void *a, size_t l, int p) { return (int)__imported_wali_mprotect(a, l, p); }
#else
#include <sys/syscall.h>
void *wali_mmap(void *a, size_t l, int p, int f, int fd, off_t o) {
    return (void *)syscall(SYS_mmap, a, l, p, f, fd, o);
}
int wali_munmap(void *a, size_t l) { return syscall(SYS_munmap, a, l); }
int wali_mprotect(void *a, size_t l, int p) { return syscall(SYS_mprotect, a, l, p); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "rw_to_ro";
    size_t size = 4096;

    if (!strcmp(mode, "bad_addr")) {
        // mprotect on never-mapped region → ENOMEM.
        // Use a high but page-aligned address.
        long r = wali_mprotect((void *)0x100000000ULL, size, PROT_READ);
        return (r < 0) ? 0 : -1;
    }

    int initial_prot = !strcmp(mode, "ro_to_rw") ? PROT_READ : (PROT_READ | PROT_WRITE);
    void *ptr = wali_mmap(NULL, size, initial_prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return -1;

    int new_prot;
    if (!strcmp(mode, "rw_to_ro"))      new_prot = PROT_READ;
    else if (!strcmp(mode, "ro_to_rw")) new_prot = PROT_READ | PROT_WRITE;
    else { wali_munmap(ptr, size); return -1; }

    long r = wali_mprotect(ptr, size, new_prot);
    int ret = (r == 0) ? 0 : -1;

    if (ret == 0 && new_prot & PROT_WRITE) {
        // Verify writability after switching to RW.
        ((char *)ptr)[0] = 'X';
        if (((char *)ptr)[0] != 'X') ret = -1;
    }
    wali_munmap(ptr, size);
    return ret;
}
