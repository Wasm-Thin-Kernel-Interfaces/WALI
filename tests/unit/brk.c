// CMD: setup="" args="" cleanup=""

// brk is a NOP in WALI; this test is a smoke check that the syscall is
// reachable and doesn't return the conventional -1 error sentinel under
// either target. Don't add growth/shrink assertions — they will diverge.

#include "wali_start.c"

#ifdef __wasm__
WALI_IMPORT("SYS_brk") long wali_syscall_brk(void *addr);
#else
#include <sys/syscall.h>
long wali_syscall_brk(void *addr) { return syscall(SYS_brk, addr); }
#endif

int test(void) {
    long curr = wali_syscall_brk(0);
    TEST_ASSERT_NE(curr, -1);
    return 0;
}
