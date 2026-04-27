// CMD: args="nonexistent_path"
// CMD: setup="/tmp/wali_execve_data" args="not_executable /tmp/wali_execve_data" cleanup="/tmp/wali_execve_data"
// CMD: args="empty_path"

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_execve")))
long __imported_wali_execve(const char *pathname, char *const argv[], char *const envp[]);
int wali_execve(const char *pathname, char *const argv[], char *const envp[]) {
    return (int)__imported_wali_execve(pathname, argv, envp);
}
#else
#include <sys/syscall.h>
int wali_execve(const char *pathname, char *const argv[], char *const envp[]) {
    return syscall(SYS_execve, pathname, argv, envp);
}
#endif

#ifdef WALI_TEST_WRAPPER
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    // Create a regular non-executable file.
    int fd = open(argv[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    unlink(argv[0]);
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 2) return -1;
    const char *mode = argv[1];

    char *exec_argv[] = { "x", NULL };
    char *exec_env[]  = { NULL };

    const char *path;
    if (!strcmp(mode, "nonexistent_path")) {
        path = "/nonexistent/path/to/exec";
    } else if (!strcmp(mode, "not_executable")) {
        if (argc < 3) return -1;
        path = argv[2];
    } else if (!strcmp(mode, "empty_path")) {
        path = "";
    } else {
        return -1;
    }

    long r = wali_execve(path, exec_argv, exec_env);
    // execve only returns on failure; success replaces the process.
    return (r < 0) ? 0 : -1;
}
