#include <stdio.h>

#include "beet/compiler/api.h"
#include "beet/diag/diag.h"
#include "beet/support/source.h"

int main(void) {
    const char *text =
        "fn main() -> Int = {\n"
        "    let x = 1\n"
        "    x +\n"
        "}\n";

    beet_source_file file = beet_source_file_from_cstr("demo.beet", text);
    beet_diag diag;

    puts(BEET_COMPILER_NAME ": stage0 compiler bootstrap");

    diag.level = BEET_DIAG_ERROR;
    diag.file = &file;
    diag.span = beet_source_span_from_offsets(&file, 35U, 36U);
    diag.message = "expected expression after binary operator";
    diag.note = "parser implementation is not added yet; this is a diagnostic smoke sample";

    beet_diag_emit(stdout, &diag, (beet_diag_options){ true });

    return 0;
}