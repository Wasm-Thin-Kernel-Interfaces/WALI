// CMD: env="TEST_VAR=hello"     args="TEST_VAR hello"
// CMD: env="TEST_VAR=world"     args="TEST_VAR world"
// CMD: env="A=foo B=bar"        args="A foo"
// CMD: env="A=foo B=bar"        args="B bar"
// CMD: env="EMPTY="             args="EMPTY ___EMPTY___"
// CMD:                           args="UNSET_VAR ___MISSING___"

#include "wali_start.c"
#include <stdlib.h>
#include <string.h>

int test(void) {
    if (test_init_args() != 0) return -1;
    test_init_env();
    if (argc < 3) return -1;
    const char *name = argv[1];
    const char *expected = argv[2];

    char *val = getenv(name);

    if (!strcmp(expected, "___MISSING___")) {
        return (val == NULL) ? 0 : -1;
    }
    if (!strcmp(expected, "___EMPTY___")) {
        return (val != NULL && val[0] == '\0') ? 0 : -1;
    }
    if (!val) return -1;
    return strcmp(val, expected) == 0 ? 0 : -1;
}
