// CMD: args="self_zero"
// CMD: args="self_explicit"
// CMD: args="bad_pid"

#define _GNU_SOURCE
#include "wali_start.c"
#include <sched.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_sched_getaffinity")))
long __imported_wali_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);
int wali_sched_getaffinity(pid_t p, size_t s, cpu_set_t *m) { return (int)__imported_wali_sched_getaffinity(p, s, m); }
pid_t wali_getpid(void) { return (pid_t)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
int wali_sched_getaffinity(pid_t p, size_t s, cpu_set_t *m) { return syscall(SYS_sched_getaffinity, p, s, m); }
pid_t wali_getpid(void) { return syscall(SYS_getpid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "self_zero";

    if (!strcmp(mode, "self_zero")) {
        cpu_set_t m;
        CPU_ZERO(&m);
        if (wali_sched_getaffinity(0, sizeof(m), &m) < 0) return -1;
        return (CPU_COUNT(&m) > 0) ? 0 : -1;
    }
    if (!strcmp(mode, "self_explicit")) {
        cpu_set_t a, b;
        CPU_ZERO(&a); CPU_ZERO(&b);
        if (wali_sched_getaffinity(0, sizeof(a), &a) < 0) return -1;
        if (wali_sched_getaffinity(wali_getpid(), sizeof(b), &b) < 0) return -1;
        return (CPU_EQUAL(&a, &b)) ? 0 : -1;
    }
    if (!strcmp(mode, "bad_pid")) {
        cpu_set_t m;
        long r = wali_sched_getaffinity(2147483640, sizeof(m), &m);
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
