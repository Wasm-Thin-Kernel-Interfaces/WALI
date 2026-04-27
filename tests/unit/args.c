// CMD: args="a1 a2"
// CMD: args="shorthand1 shorthand2"
// CMD: args="multi1 multi2 multi3 multi4 multi5"
// CMD: args="'spaces are ok' end"

#include "wali_start.c"
#include <string.h>

int test(void) {
    if (test_init_args() != 0) return -1;
    if (argc < 2) return -1;

    const char *first = argv[1];

    if (!strcmp(first, "a1")) {
        if (argc != 3) return -1;
        if (strcmp(argv[2], "a2") != 0) return -1;
    } else if (!strcmp(first, "shorthand1")) {
        if (argc != 3) return -1;
        if (strcmp(argv[2], "shorthand2") != 0) return -1;
    } else if (!strcmp(first, "multi1")) {
        if (argc != 6) return -1;
        if (strcmp(argv[2], "multi2") != 0) return -1;
        if (strcmp(argv[3], "multi3") != 0) return -1;
        if (strcmp(argv[4], "multi4") != 0) return -1;
        if (strcmp(argv[5], "multi5") != 0) return -1;
    } else if (!strcmp(first, "spaces are ok")) {
        if (argc != 3) return -1;
        if (strcmp(argv[2], "end") != 0) return -1;
    } else {
        return -1;
    }

    return 0;
}
