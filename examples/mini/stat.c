#include "common.h"


int main()
{
    struct stat sb;
    char path[] = "compile-wali-debug.sh";

    if (stat(path, &sb) == -1) {
        print("ERROR\n");
        exit(1);
    }

    printf("Size of stat var: %lu\n", sizeof(sb));

    print("ID of containing device: [");
    print_int((uintmax_t) major(sb.st_dev));
    print(",");
    print_int((uintmax_t) minor(sb.st_dev));
    print("]\n");

    print("File type:                ");

    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:  print("block device\n");            break;
    case S_IFCHR:  print("character device\n");        break;
    case S_IFDIR:  print("directory\n");               break;
    case S_IFIFO:  print("FIFO/pipe\n");               break;
    case S_IFLNK:  print("symlink\n");                 break;
    case S_IFREG:  print("regular file\n");            break;
    case S_IFSOCK: print("socket\n");                  break;
    default:       print("unknown?\n");                break;
    }

    PRINT_INT("I-node number", (uintmax_t) sb.st_ino);
    PRINT_INT("Mode", (uintmax_t) sb.st_mode);
    PRINT_INT("Link count", (uintmax_t) sb.st_nlink);
    PRINT_INT("UID", (uintmax_t) sb.st_uid);
    PRINT_INT("GID", (uintmax_t) sb.st_gid);
    PRINT_INT("Preferred I/O block size", (intmax_t) sb.st_blksize);
    PRINT_INT("File size", (intmax_t) sb.st_size);
    PRINT_INT("Blocks allocated", (intmax_t) sb.st_blocks);
    PRINT_STR("Last status change", ctime(&sb.st_ctime));
    PRINT_STR("Last file access", ctime(&sb.st_atime));
    PRINT_STR("Last file modification", ctime(&sb.st_mtime));

    //exit(EXIT_SUCCESS);
}
