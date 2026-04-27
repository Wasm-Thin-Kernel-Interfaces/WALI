// CMD: args="set_get"
// CMD: args="disable"
// CMD: args="query_only"

#include "wali_start.c"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_sigaltstack")))
long __imported_wali_sigaltstack(const stack_t *ss, stack_t *old_ss);
int wali_sigaltstack(const stack_t *s, stack_t *o) { return (int)__imported_wali_sigaltstack(s, o); }
#else
#include <sys/syscall.h>
int wali_sigaltstack(const stack_t *s, stack_t *o) { return syscall(SYS_sigaltstack, s, o); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "set_get";

    char *mem = (char *)malloc(SIGSTKSZ);
    if (!mem) return -1;

    int ret = -1;
    if (!strcmp(mode, "set_get")) {
        stack_t ss = { .ss_sp = mem, .ss_size = SIGSTKSZ, .ss_flags = 0 };
        if (wali_sigaltstack(&ss, NULL) != 0) goto out;
        stack_t got;
        memset(&got, 0, sizeof(got));
        if (wali_sigaltstack(NULL, &got) != 0) goto out;
        ret = (got.ss_sp == mem && got.ss_size == SIGSTKSZ) ? 0 : -1;
        // Disable so the freed memory isn't kept around as the alt stack.
        ss.ss_flags = SS_DISABLE;
        wali_sigaltstack(&ss, NULL);
    } else if (!strcmp(mode, "disable")) {
        stack_t off = { .ss_flags = SS_DISABLE };
        ret = (wali_sigaltstack(&off, NULL) == 0) ? 0 : -1;
    } else if (!strcmp(mode, "query_only")) {
        stack_t got;
        memset(&got, 0, sizeof(got));
        ret = (wali_sigaltstack(NULL, &got) == 0) ? 0 : -1;
    }

out:
    free(mem);
    return ret;
}
