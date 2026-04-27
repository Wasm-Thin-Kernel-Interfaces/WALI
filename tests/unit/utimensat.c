// CMD: setup="/tmp/uts_a"  args="set       1000 2000 /tmp/uts_a"  cleanup="atime:1000 mtime:2000 /tmp/uts_a"
// CMD: setup="/tmp/uts_b"  args="set       50   60   /tmp/uts_b"  cleanup="atime:50 mtime:60 /tmp/uts_b"
// CMD:                      args="missing   0    0    /tmp/uts_no" cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <time.h>
#include <string.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_utimensat")))
long __imported_wali_utimensat(int dirfd, const char *pathname, const struct timespec *times, int flags);
int wali_utimensat(int d, const char *p, const struct timespec *t, int f) {
    return (int)__imported_wali_utimensat(d, p, t, f);
}
#else
#include <sys/syscall.h>
int wali_utimensat(int d, const char *p, const struct timespec *t, int f) {
    return syscall(SYS_utimensat, d, p, t, f);
}
#endif

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
#include <stdlib.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    int fd = open(argv[0], O_WRONLY | O_CREAT, 0644);
    if (fd >= 0) close(fd);
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc == 0) return 0;
    if (argc == 1) { unlink(argv[0]); return 0; }
    // Verify atime/mtime then unlink. Args layout: "atime:<sec> mtime:<sec> path".
    long want_atime = -1, want_mtime = -1;
    const char *path = NULL;
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "atime:", 6)) want_atime = atol(argv[i] + 6);
        else if (!strncmp(argv[i], "mtime:", 6)) want_mtime = atol(argv[i] + 6);
        else path = argv[i];
    }
    if (!path) return -1;
    struct stat st;
    int ok = (stat(path, &st) == 0)
          && (want_atime < 0 || st.st_atime == want_atime)
          && (want_mtime < 0 || st.st_mtime == want_mtime);
    unlink(path);
    return ok ? 0 : -1;
}
#endif

static long parse_long(const char *s) {
    long v = 0;
    for (; *s >= '0' && *s <= '9'; s++) v = v * 10 + (*s - '0');
    return v;
}

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 5) return -1;
    const char *mode = argv[1];
    long atime = parse_long(argv[2]);
    long mtime = parse_long(argv[3]);
    const char *path = argv[4];

    if (!strcmp(mode, "missing")) {
        long r = wali_utimensat(AT_FDCWD, path, NULL, 0);
        return (r < 0) ? 0 : -1;
    }

    struct timespec ts[2];
    ts[0].tv_sec = atime; ts[0].tv_nsec = 0;
    ts[1].tv_sec = mtime; ts[1].tv_nsec = 0;
    long r = wali_utimensat(AT_FDCWD, path, ts, 0);
    return (r == 0) ? 0 : -1;
}
