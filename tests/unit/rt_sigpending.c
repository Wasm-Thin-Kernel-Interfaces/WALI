// CMD: args="pending"
// CMD: args="empty"

#include "wali_start.c"
#include <signal.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_rt_sigpending")))
long __imported_wali_rt_sigpending(sigset_t *set, size_t sigsetsize);
__attribute__((__import_module__("wali"), __import_name__("SYS_rt_sigprocmask")))
long __imported_wali_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize);
__attribute__((__import_module__("wali"), __import_name__("SYS_kill")))
long __imported_wali_kill(pid_t pid, int sig);
__attribute__((__import_module__("wali"), __import_name__("SYS_getpid")))
long __imported_wali_getpid(void);
int wali_rt_sigpending(sigset_t *s, size_t sz) { return (int)__imported_wali_rt_sigpending(s, sz); }
int wali_rt_sigprocmask(int h, const sigset_t *s, sigset_t *o, size_t sz) { return (int)__imported_wali_rt_sigprocmask(h, s, o, sz); }
int wali_kill(pid_t p, int s) { return (int)__imported_wali_kill(p, s); }
pid_t wali_getpid(void) { return (pid_t)__imported_wali_getpid(); }
#else
#include <sys/syscall.h>
int wali_rt_sigpending(sigset_t *s, size_t sz) { return syscall(SYS_rt_sigpending, s, sz); }
int wali_rt_sigprocmask(int h, const sigset_t *s, sigset_t *o, size_t sz) { return syscall(SYS_rt_sigprocmask, h, s, o, sz); }
int wali_kill(pid_t p, int s) { return syscall(SYS_kill, p, s); }
pid_t wali_getpid(void) { return syscall(SYS_getpid); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "pending";

    if (!strcmp(mode, "empty")) {
        // Pending set with nothing queued must report no member.
        sigset_t pending;
        sigemptyset(&pending);
        if (wali_rt_sigpending(&pending, sizeof(sigset_t)) != 0) return -1;
        return (sigismember(&pending, SIGUSR1) == 0 && sigismember(&pending, SIGUSR2) == 0) ? 0 : -1;
    }

    if (!strcmp(mode, "pending")) {
        sigset_t oldmask, block;
        sigemptyset(&block);
        sigaddset(&block, SIGUSR1);
        if (wali_rt_sigprocmask(SIG_BLOCK, &block, &oldmask, sizeof(sigset_t)) != 0) return -1;
        if (wali_kill(wali_getpid(), SIGUSR1) != 0) {
            wali_rt_sigprocmask(SIG_SETMASK, &oldmask, NULL, sizeof(sigset_t));
            return -1;
        }
        sigset_t pending;
        sigemptyset(&pending);
        int rp = wali_rt_sigpending(&pending, sizeof(sigset_t));
        int is_pending = sigismember(&pending, SIGUSR1);
        // Discard SIGUSR1 before unblocking.
        struct sigaction sa = {0}; sa.sa_handler = SIG_IGN;
        sigaction(SIGUSR1, &sa, NULL);
        wali_rt_sigprocmask(SIG_SETMASK, &oldmask, NULL, sizeof(sigset_t));
        return (rp == 0 && is_pending) ? 0 : -1;
    }
    return -1;
}
