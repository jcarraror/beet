#include <stdio.h>
#include <stdlib.h>

#include "beet/compiler/api.h"
#include "beet/diag/diag.h"
#include "beet/support/source.h"

int main(void) {
    const char *text =
        "fn main() -> Int = {\n"
        "    let x = 1\n"
        "    x +\n"
        "}\n";

    beet_source_file file;
    beet_diag diag;
    int exit_code;

    beet_source_file_init(&file);
    exit_code = 0;

    if (!beet_source_file_set_text_copy(&file, "demo.beet", text)) {
        fputs("failed to initialize source file\n", stderr);
        return 1;
    }

    puts(BEET_COMPILER_NAME ": stage0 compiler bootstrap");

    diag.level = BEET_DIAG_ERROR;
    diag.file = &file;
    diag.span = beet_source_span_from_offsets(&file, 35U, 36U);
    diag.message = "expected expression after binary operator";
    diag.note = "parser implementation is not added yet; this is a diagnostic smoke sample";

    beet_diag_emit(stdout, &diag, (beet_diag_options){ true });

    beet_source_file_dispose(&file);
    return exit_code;
}