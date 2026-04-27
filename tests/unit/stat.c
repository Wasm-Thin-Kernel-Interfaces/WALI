// CMD: setup="file:/tmp/stat_a 100" args="file 100 /tmp/stat_a" cleanup="/tmp/stat_a"
// CMD: setup="dir:/tmp/stat_d"        args="dir 0 /tmp/stat_d"   cleanup="dir:/tmp/stat_d"
// CMD:                                  args="missing 0 /tmp/stat_no" cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_stat")))
long __imported_wali_stat(const char *pathname, struct stat *statbuf);
int wali_stat(const char *path, struct stat *st) { return (int)__imported_wali_stat(path, st); }
#else
int wali_stat(const char *path, struct stat *st) { return wali_syscall_stat(path, st); }
#endif

#ifdef WALI_TEST_WRAPPER
#include <stdio.h>
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    if (!strncmp(argv[0], "file:", 5)) {
        if (argc < 2) return -1;
        int size = atoi(argv[1]);
        int fd = open(argv[0] + 5, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return -1;
        char buf[256] = {0};
        memset(buf, 'x', sizeof(buf));
        for (int i = 0; i < size; i += 256) {
            int chunk = (size - i > 256) ? 256 : (size - i);
            write(fd, buf, chunk);
        }
        close(fd);
    } else if (!strncmp(argv[0], "dir:", 4)) {
        rmdir(argv[0] + 4);
        return mkdir(argv[0] + 4, 0755);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "dir:", 4)) rmdir(argv[i] + 4);
        else unlink(argv[i]);
    }
    return 0;
}
#endif

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 4) return -1;
    const char *mode = argv[1];
    long expected_size = 0;
    for (const char *p = argv[2]; *p; p++) expected_size = expected_size * 10 + (*p - '0');
    const char *path = argv[3];

    struct stat st;
    long r = wali_stat(path, &st);

    if (!strcmp(mode, "missing")) return (r < 0) ? 0 : -1;
    if (r != 0) return -1;
    if (!strcmp(mode, "file")) return (S_ISREG(st.st_mode) && st.st_size == expected_size) ? 0 : -1;
    if (!strcmp(mode, "dir"))  return S_ISDIR(st.st_mode) ? 0 : -1;
    return -1;
}
