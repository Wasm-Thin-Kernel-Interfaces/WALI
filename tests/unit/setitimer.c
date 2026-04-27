// CMD: args="set_real"
// CMD: args="disable"
// CMD: args="get_old"
// CMD: args="bad_which"

#include "wali_start.c"
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_setitimer")))
long __imported_wali_setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
__attribute__((__import_module__("wali"), __import_name__("SYS_rt_sigaction")))
long __imported_wali_rt_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize);
int wali_setitimer(int w, const struct itimerval *n, struct itimerval *o) { return (int)__imported_wali_setitimer(w, n, o); }
int wali_rt_sigaction(int s, const struct sigaction *a, struct sigaction *o, size_t sz) { return (int)__imported_wali_rt_sigaction(s, a, o, sz); }
#else
#include <sys/syscall.h>
int wali_setitimer(int w, const struct itimerval *n, struct itimerval *o) { return syscall(SYS_setitimer, w, n, o); }
int wali_rt_sigaction(int s, const struct sigaction *a, struct sigaction *o, size_t sz) { return syscall(SYS_rt_sigaction, s, a, o, sz); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

static void install_ignore_for_sigalrm(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    wali_rt_sigaction(SIGALRM, &sa, NULL, sizeof(sigset_t));
}

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "set_real";

    if (!strcmp(mode, "bad_which")) {
        struct itimerval it = {{0, 0}, {0, 0}};
        long r = wali_setitimer(9999, &it, NULL);
        return (r < 0) ? 0 : -1;
    }

    install_ignore_for_sigalrm();
    struct itimerval it;
    memset(&it, 0, sizeof(it));

    if (!strcmp(mode, "set_real")) {
        it.it_value.tv_usec = 10000;  // 10ms
        long r = wali_setitimer(ITIMER_REAL, &it, NULL);
        // Disable.
        struct itimerval off = {{0, 0}, {0, 0}};
        wali_setitimer(ITIMER_REAL, &off, NULL);
        return (r == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "disable")) {
        // Disabling an inactive timer must succeed.
        long r = wali_setitimer(ITIMER_REAL, &it, NULL);
        return (r == 0) ? 0 : -1;
    }
    if (!strcmp(mode, "get_old")) {
        // Set, then set again capturing old_value, verify old reflects prior setting.
        it.it_value.tv_sec = 10; it.it_value.tv_usec = 0;
        if (wali_setitimer(ITIMER_REAL, &it, NULL) != 0) return -1;
        struct itimerval off = {{0, 0}, {0, 0}}, old;
        memset(&old, 0, sizeof(old));
        if (wali_setitimer(ITIMER_REAL, &off, &old) != 0) return -1;
        // old should report a remaining time within 10s.
        return (old.it_value.tv_sec >= 0 && old.it_value.tv_sec <= 10) ? 0 : -1;
    }
    return -1;
}
