// CMD: args="wrong_value"
// CMD: args="timeout"
// CMD: args="wake_zero"

#include "wali_start.c"
#include <linux/futex.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_futex")))
long __imported_wali_futex(int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3);
int wali_futex(int *uaddr, int op, int val, const struct timespec *t, int *uaddr2, int val3) {
    return (int)__imported_wali_futex(uaddr, op, val, t, uaddr2, val3);
}
#else
#include <sys/syscall.h>
int wali_futex(int *uaddr, int op, int val, const struct timespec *t, int *uaddr2, int val3) {
    return syscall(SYS_futex, uaddr, op, val, t, uaddr2, val3);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "wrong_value";

    int futex_word = 0;
    struct timespec timeout = {0, 1000000};  // 1ms

    if (!strcmp(mode, "wrong_value")) {
        // val mismatch (uaddr=0, val=1) → returns immediately with EAGAIN.
        long r = wali_futex(&futex_word, FUTEX_WAIT, 1, &timeout, NULL, 0);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "timeout")) {
        // val matches → blocks until 1ms timeout.
        long r = wali_futex(&futex_word, FUTEX_WAIT, 0, &timeout, NULL, 0);
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "wake_zero")) {
        // No waiters → returns 0 (count of woken).
        long r = wali_futex(&futex_word, FUTEX_WAKE, 1, NULL, NULL, 0);
        return (r == 0) ? 0 : -1;
    }
    return -1;
}
