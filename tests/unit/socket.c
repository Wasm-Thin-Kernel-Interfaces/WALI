// CMD: args="udp"
// CMD: args="tcp"
// CMD: args="unix_stream"
// CMD: args="unix_dgram"
// CMD: args="bad_domain"
// CMD: args="bad_type"

#include "wali_start.c"
#include <sys/socket.h>
#include <string.h>

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) { return 0; }
int test_cleanup(int argc, char **argv) { return 0; }
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    const char *mode = (argc > 1) ? argv[1] : "udp";

    int domain, type;
    int expect_ok = 1;
    if (!strcmp(mode, "udp"))         { domain = AF_INET;  type = SOCK_DGRAM; }
    else if (!strcmp(mode, "tcp"))    { domain = AF_INET;  type = SOCK_STREAM; }
    else if (!strcmp(mode, "unix_stream")) { domain = AF_UNIX; type = SOCK_STREAM; }
    else if (!strcmp(mode, "unix_dgram"))  { domain = AF_UNIX; type = SOCK_DGRAM; }
    else if (!strcmp(mode, "bad_domain"))  { domain = 9999; type = SOCK_STREAM; expect_ok = 0; }
    else if (!strcmp(mode, "bad_type"))    { domain = AF_INET; type = 9999; expect_ok = 0; }
    else return -1;

    long fd = wali_syscall_socket(domain, type, 0);
    int success = (fd >= 0);
    if (success) wali_syscall_close((int)fd);
    return (success == expect_ok) ? 0 : -1;
}
