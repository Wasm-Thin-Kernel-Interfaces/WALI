// CMD: args="unix_stream"
// CMD: args="unix_dgram"
// CMD: args="bad_domain"

#include "wali_start.c"
#include <sys/socket.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_socketpair")))
long __imported_wali_socketpair(int domain, int type, int protocol, int *sv);
int wali_socketpair(int d, int t, int p, int *sv) { return (int)__imported_wali_socketpair(d, t, p, sv); }
#else
#include <sys/syscall.h>
int wali_socketpair(int d, int t, int p, int *sv) { return syscall(SYS_socketpair, d, t, p, sv); }
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "unix_stream";

    int domain, type;
    int expect_ok = 1;
    if (!strcmp(mode, "unix_stream"))      { domain = AF_UNIX; type = SOCK_STREAM; }
    else if (!strcmp(mode, "unix_dgram"))  { domain = AF_UNIX; type = SOCK_DGRAM; }
    else if (!strcmp(mode, "bad_domain"))  { domain = AF_INET; type = SOCK_STREAM; expect_ok = 0; }
    else return -1;

    int sv[2];
    long r = wali_socketpair(domain, type, 0, sv);
    int success = (r == 0);
    if (success) {
        wali_syscall_close(sv[0]);
        wali_syscall_close(sv[1]);
    }
    return (success == expect_ok) ? 0 : -1;
}
