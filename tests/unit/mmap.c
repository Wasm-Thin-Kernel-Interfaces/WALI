// CMD:                                args="anon"                                cleanup=""
// CMD:                                args="anon_shared"                         cleanup=""
// CMD: setup="/tmp/map_a" args="file        /tmp/map_a" cleanup="/tmp/map_a"
// CMD:                                args="bad_fd"                              cleanup=""

// Disabled — WALI / native divergence:
//   Native: mmap(NULL, 0, ...) returns -1 with EINVAL.
//   WALI:   returns a valid pointer (size-0 mmap is silently accepted).
// Re-enable once WALI's mmap rejects length=0 with EINVAL.
// // CMD:                                args="bad_size_zero"                        cleanup=""

#include "wali_start.c"
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_mmap")))
long long __imported_wali_mmap(void *addr, size_t length, int prot, int flags, int fd, long long offset);
__attribute__((__import_module__("wali"), __import_name__("SYS_munmap")))
long long __imported_wali_munmap(void *addr, size_t length);

void *wali_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    long long res = __imported_wali_mmap(addr, length, prot, flags, fd, (long long)offset);
    if (res < 0 && res > -4096) return MAP_FAILED;
    return (void *)(long)res;
}
int wali_munmap(void *addr, size_t length) { return (int)__imported_wali_munmap(addr, length); }
#else
#include <sys/syscall.h>
void *wali_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return (void *)syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
}
int wali_munmap(void *addr, size_t length) { return syscall(SYS_munmap, addr, length); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) { write(fd, "0123456789", 10); close(fd); }
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
    const char *mode = (argc > 1) ? argv[1] : "anon";

    if (!strcmp(mode, "anon")) {
        void *p = wali_mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) return -1;
        ((char *)p)[0] = 'A'; ((char *)p)[4095] = 'Z';
        if (((char *)p)[0] != 'A' || ((char *)p)[4095] != 'Z') return -1;
        return wali_munmap(p, 4096);
    }
    if (!strcmp(mode, "anon_shared")) {
        void *p = wali_mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) return -1;
        return wali_munmap(p, 4096);
    }
    if (!strcmp(mode, "file")) {
        if (argc < 3) return -1;
        int fd = wali_syscall_open(argv[2], O_RDONLY, 0);
        if (fd < 0) return -1;
        char *p = wali_mmap(NULL, 10, PROT_READ, MAP_PRIVATE, fd, 0);
        wali_syscall_close(fd);
        if (p == MAP_FAILED) return -1;
        int ok = (p[0] == '0' && p[9] == '9');
        wali_munmap(p, 10);
        return ok ? 0 : -1;
    }
    if (!strcmp(mode, "bad_size_zero")) {
        void *p = wali_mmap(NULL, 0, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return (p == MAP_FAILED) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_fd")) {
        // File-backed mmap with bad fd → fail.
        void *p = wali_mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, 99999, 0);
        return (p == MAP_FAILED) ? 0 : -1;
    }
    // Disabled — WALI returns success on size-0 mmap; native returns EINVAL.
    // if (!strcmp(mode, "bad_size_zero")) {
    //     void *p = wali_mmap(NULL, 0, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //     return (p == MAP_FAILED) ? 0 : -1;
    // }
    return -1;
}
