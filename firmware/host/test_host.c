#include <stdio.h>

#include "selftest.h"

static void emit_stdout(const char *line) {
    puts(line);
}

int main(void) {
    selftest_run(emit_stdout);
    return 0;
}
