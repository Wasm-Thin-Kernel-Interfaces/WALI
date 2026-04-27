// CMD: setup="create /tmp/open_read.txt" args="read /tmp/open_read.txt" cleanup="/tmp/open_read.txt"
// CMD: setup="clean /tmp/open_creat.txt" args="create /tmp/open_creat.txt" cleanup="/tmp/open_creat.txt"
// CMD: setup="clean /tmp/open_fail.txt" args="fail /tmp/open_fail.txt" cleanup="/tmp/open_fail.txt"
// CMD: setup="create /tmp/open_append.txt" args="append /tmp/open_append.txt" cleanup="/tmp/open_append.txt"
// CMD: setup="clean /tmp/open_excl.txt" args="excl_ok /tmp/open_excl.txt" cleanup="/tmp/open_excl.txt"
// CMD: setup="create /tmp/open_excl_fail.txt" args="excl_fail /tmp/open_excl_fail.txt" cleanup="/tmp/open_excl_fail.txt"
// CMD: setup="create_dir /tmp/open_dir" args="directory_ok /tmp/open_dir" cleanup="/tmp/open_dir"
// CMD: setup="create /tmp/open_dir_fail.txt" args="directory_fail /tmp/open_dir_fail.txt" cleanup="/tmp/open_dir_fail.txt"

#include "wali_start.c"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WALI_TEST_WRAPPER
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// Helper to check file existence
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int test_setup(int argc, char **argv) {
    if (argc < 2) return -1;
    const char *mode = argv[0];
    const char *path = argv[1];
    
    if (strcmp(mode, "create") == 0) {
        // Create file for reading test
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
             perror("Setup open failed");
             return -1;
        }
        if (write(fd, "SETUP_DATA", 10) != 10) {
            perror("Setup write failed");
            close(fd);
            return -1;
        }
        close(fd);
    } else if (strcmp(mode, "clean") == 0) {
        // Ensure file does not exist
        unlink(path);
    } else if (strcmp(mode, "create_dir") == 0) {
        mkdir(path, 0755);
    }
    return 0;
}

int test_cleanup(int argc, char **argv) {
    if (argc < 1) return -1;
    const char *path = argv[0];
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) rmdir(path);
        else unlink(path);
    }
    return 0;
}
#endif

#ifdef __wasm__
__attribute__((__import_module__("wali"), __import_name__("SYS_open")))
long __imported_wali_open(const char *pathname, int flags, int mode);

__attribute__((__import_module__("wali"), __import_name__("SYS_close")))
long __imported_wali_close(int fd);

int wali_open(const char *pathname, int flags, int mode) {
  return (int)__imported_wali_open(pathname, flags, mode);
}
int wali_close(int fd) {
    return (int)__imported_wali_close(fd);
}
#else
int wali_open(const char *pathname, int flags, int mode) { return wali_syscall_open(pathname, flags, mode); }
int wali_close(int fd) { return wali_syscall_close(fd); }
#endif

int test(void) {
  if (test_init_args() != 0) return -1;

  if (argc < 3) return -1;
  const char *mode = argv[1];
  const char *path = argv[2];
  
  int flags = 0;
  int expected_success = 1;
  
  if (strcmp(mode, "read") == 0) {
      flags = O_RDONLY;
      expected_success = 1;
  } else if (strcmp(mode, "create") == 0) {
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      expected_success = 1;
  } else if (strcmp(mode, "fail") == 0) {
      flags = O_RDONLY;
      expected_success = 0;
  } else if (strcmp(mode, "append") == 0) {
      flags = O_WRONLY | O_APPEND;
      expected_success = 1;
  } else if (strcmp(mode, "excl_ok") == 0) {
      flags = O_WRONLY | O_CREAT | O_EXCL;
      expected_success = 1;
  } else if (strcmp(mode, "excl_fail") == 0) {
      flags = O_WRONLY | O_CREAT | O_EXCL;
      expected_success = 0;
  } else if (strcmp(mode, "directory_ok") == 0) {
      flags = O_RDONLY | O_DIRECTORY;
      expected_success = 1;
  } else if (strcmp(mode, "directory_fail") == 0) {
      flags = O_RDONLY | O_DIRECTORY;
      expected_success = 0;
  } else {
      return -1; 
  }

  int fd = wali_open(path, flags, 0644);
  int success = (fd >= 0);
  
  int result = -1;
  if (expected_success) {
      if (success) {
          result = 0;
          wali_close(fd);
      } else {
          result = -1;
      }
  } else {
      if (!success) {
          result = 0; // successfully failed
      } else {
          result = -1; // unexpectedly succeeded
          wali_close(fd);
      }
  }
  
  return result;
}
