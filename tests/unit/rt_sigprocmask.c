// CMD: args="block_setmask"
// CMD: args="query_only"
// CMD: args="unblock"
// CMD: args="bad_how"

#include "wali_start.c"
#include <signal.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_rt_sigprocmask")))
long __imported_wali_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize);
int wali_rt_sigprocmask(int h, const sigset_t *s, sigset_t *o, size_t sz) { return (int)__imported_wali_rt_sigprocmask(h, s, o, sz); }
#else
#include <sys/syscall.h>
int wali_rt_sigprocmask(int h, const sigset_t *s, sigset_t *o, size_t sz) { return syscall(SYS_rt_sigprocmask, h, s, o, sz); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "block_setmask";
    sigset_t set, old;

    if (!strcmp(mode, "block_setmask")) {
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        if (wali_rt_sigprocmask(SIG_BLOCK, &set, &old, sizeof(sigset_t)) != 0) return -1;
        sigset_t cur;
        if (wali_rt_sigprocmask(SIG_BLOCK, NULL, &cur, sizeof(sigset_t)) != 0) return -1;
        int ok = sigismember(&cur, SIGUSR1);
        wali_rt_sigprocmask(SIG_SETMASK, &old, NULL, sizeof(sigset_t));
        return ok ? 0 : -1;
    }
    if (!strcmp(mode, "query_only")) {
        // set=NULL: pure query.
        if (wali_rt_sigprocmask(0, NULL, &old, sizeof(sigset_t)) != 0) return -1;
        return 0;
    }
    if (!strcmp(mode, "unblock")) {
        sigemptyset(&set);
        sigaddset(&set, SIGUSR2);
        if (wali_rt_sigprocmask(SIG_BLOCK, &set, &old, sizeof(sigset_t)) != 0) return -1;
        if (wali_rt_sigprocmask(SIG_UNBLOCK, &set, NULL, sizeof(sigset_t)) != 0) return -1;
        sigset_t cur;
        if (wali_rt_sigprocmask(SIG_BLOCK, NULL, &cur, sizeof(sigset_t)) != 0) return -1;
        int ok = !sigismember(&cur, SIGUSR2);
        wali_rt_sigprocmask(SIG_SETMASK, &old, NULL, sizeof(sigset_t));
        return ok ? 0 : -1;
    }
    if (!strcmp(mode, "bad_how")) {
        sigemptyset(&set);
        long r = wali_rt_sigprocmask(9999, &set, NULL, sizeof(sigset_t));
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
