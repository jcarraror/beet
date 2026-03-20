#include <stdio.h>

#include "beet/diag/diag.h"
#include "beet/support/source.h"

int main(void) {
    const char *text =
        "fn main() -> Int = {\n"
        "    let x = 10\n"
        "    x +\n"
        "}\n";

    beet_source_file file;
    beet_diag diag;
    int ok;

    beet_source_file_init(&file);

    ok = beet_source_file_set_text_copy(&file, "test.beet", text) ? 1 : 0;
    if (!ok) {
        fputs("failed to initialize test source\n", stderr);
        return 1;
    }

    diag.level = BEET_DIAG_ERROR;
    diag.file = &file;
    diag.span = beet_source_span_from_offsets(&file, 33U, 34U);
    diag.message = "expected expression after '+'";
    diag.note = "this is a diagnostic smoke test";

    beet_diag_emit(stdout, &diag, (beet_diag_options){ false });

    beet_source_file_dispose(&file);
    return 0;
}