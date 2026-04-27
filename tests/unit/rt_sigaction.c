// CMD: args="ignore"
// CMD: args="default"
// CMD: args="query_old"
// CMD: args="invalid_sigkill"
// CMD: args="invalid_signum"

#include "wali_start.c"
#include <signal.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_rt_sigaction")))
long __imported_wali_rt_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize);
__attribute__((__import_module__("wali"), __import_name__("SYS_kill")))
long __imported_wali_kill(pid_t pid, int sig);
int wali_rt_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize) {
    return (int)__imported_wali_rt_sigaction(signum, act, oldact, sigsetsize);
}
int wali_kill(pid_t pid, int sig) { return (int)__imported_wali_kill(pid, sig); }
#else
#include <sys/syscall.h>
int wali_rt_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize) {
    return syscall(SYS_rt_sigaction, signum, act, oldact, sigsetsize);
}
int wali_kill(pid_t pid, int sig) { return syscall(SYS_kill, pid, sig); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "ignore";

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    if (!strcmp(mode, "ignore")) {
        sa.sa_handler = SIG_IGN;
        if (wali_rt_sigaction(SIGUSR1, &sa, NULL, sizeof(sigset_t)) != 0) return -1;
        // Send signal — handler is SIG_IGN, we survive.
        return (wali_kill(0, SIGUSR1) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "default")) {
        // Set then reset to SIG_DFL. Don't actually deliver the signal.
        sa.sa_handler = SIG_IGN;
        if (wali_rt_sigaction(SIGUSR1, &sa, NULL, sizeof(sigset_t)) != 0) return -1;
        sa.sa_handler = SIG_DFL;
        return (wali_rt_sigaction(SIGUSR1, &sa, NULL, sizeof(sigset_t)) == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "query_old")) {
        sa.sa_handler = SIG_IGN;
        if (wali_rt_sigaction(SIGUSR2, &sa, NULL, sizeof(sigset_t)) != 0) return -1;
        struct sigaction old;
        memset(&old, 0, sizeof(old));
        if (wali_rt_sigaction(SIGUSR2, NULL, &old, sizeof(sigset_t)) != 0) return -1;
        return (old.sa_handler == SIG_IGN) ? 0 : -1;
    }
    if (!strcmp(mode, "invalid_sigkill")) {
        sa.sa_handler = SIG_IGN;
        long r = wali_rt_sigaction(SIGKILL, &sa, NULL, sizeof(sigset_t));
        return (r < 0) ? 0 : -1;
    }
    if (!strcmp(mode, "invalid_signum")) {
        sa.sa_handler = SIG_IGN;
        long r = wali_rt_sigaction(9999, &sa, NULL, sizeof(sigset_t));
        return (r < 0) ? 0 : -1;
    }
    return -1;
}
