// CMD: setup="files:/tmp/getdents_a"  args="files /tmp/getdents_a"  cleanup="files:/tmp/getdents_a"
// CMD: setup="empty:/tmp/getdents_b"  args="empty /tmp/getdents_b"  cleanup="empty:/tmp/getdents_b"
// CMD:                                  args="bad_fd /tmp/none"        cleanup=""

#include "wali_start.c"
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_getdents64")))
long __imported_wali_getdents64(int fd, void *dirp, int count);
int wali_getdents64(int fd, void *dirp, int count) { return (int)__imported_wali_getdents64(fd, dirp, count); }
#else
#include <sys/syscall.h>
int wali_getdents64(int fd, void *dirp, int count) { return syscall(SYS_getdents64, fd, dirp, count); }
#endif

#ifdef WALI_TEST_WRAPPER
#include <sys/stat.h>
#include <stdio.h>
int test_setup(int argc, char **argv) {
    if (argc < 1) return 0;
    const char *spec = argv[0];
    const char *colon = strchr(spec, ':');
    if (!colon) return 0;
    const char *kind = spec;
    int klen = colon - spec;
    const char *dir = colon + 1;

    rmdir(dir);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s/f1", dir); unlink(buf);
    snprintf(buf, sizeof(buf), "%s/f2", dir); unlink(buf);
    if (mkdir(dir, 0755) != 0) return -1;

    if (!strncmp(kind, "files", klen)) {
        snprintf(buf, sizeof(buf), "%s/f1", dir);
        int fd = open(buf, O_WRONLY | O_CREAT, 0644);
        if (fd >= 0) close(fd);
        snprintf(buf, sizeof(buf), "%s/f2", dir);
        fd = open(buf, O_WRONLY | O_CREAT, 0644);
        if (fd >= 0) close(fd);
    }
    return 0;
}
int test_cleanup(int argc, char **argv) {
    if (argc < 1) return 0;
    const char *spec = argv[0];
    const char *colon = strchr(spec, ':');
    if (!colon) return 0;
    const char *dir = colon + 1;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s/f1", dir); unlink(buf);
    snprintf(buf, sizeof(buf), "%s/f2", dir); unlink(buf);
    rmdir(dir);
    return 0;
}
#endif

struct linux_dirent64 {
    uint64_t        d_ino;
    int64_t         d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char            d_name[];
};

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 3) return -1;
    const char *mode = argv[1];
    const char *path = argv[2];

    if (!strcmp(mode, "bad_fd")) {
        char buf[1024];
        long r = wali_getdents64(99999, buf, sizeof(buf));
        return (r < 0) ? 0 : -1;
    }

    int fd = wali_syscall_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) return -1;

    char buf[1024];
    long nread = wali_getdents64(fd, buf, sizeof(buf));
    int ret = -1;

    if (!strcmp(mode, "files")) {
        if (nread <= 0) goto out;
        int f1 = 0, f2 = 0;
        long bpos = 0;
        while (bpos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + bpos);
            if (!strcmp(d->d_name, "f1")) f1 = 1;
            if (!strcmp(d->d_name, "f2")) f2 = 1;
            bpos += d->d_reclen;
        }
        ret = (f1 && f2) ? 0 : -1;
    } else if (!strcmp(mode, "empty")) {
        // Empty dir still has "." and ".." entries, but no regular files.
        if (nread < 0) goto out;
        long bpos = 0;
        int has_regular = 0;
        while (bpos < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + bpos);
            if (strcmp(d->d_name, ".") && strcmp(d->d_name, "..")) has_regular = 1;
            bpos += d->d_reclen;
        }
        ret = has_regular ? -1 : 0;
    }

out:
    wali_syscall_close(fd);
    return ret;
}
