#include "beet/diag/diag.h"
#include "beet/support/source.h"

int main(void) {
    const char *text =
        "fn main() -> Int = {\n"
        "    let x = 10\n"
        "    x +\n"
        "}\n";

    beet_source_file file = beet_source_file_from_cstr("test.beet", text);
    beet_diag diag;

    diag.level = BEET_DIAG_ERROR;
    diag.file = &file;
    diag.span = beet_source_span_from_offsets(&file, 33U, 34U);
    diag.message = "expected expression after '+'";
    diag.note = "this is a golden-style print smoke test";

    beet_diag_emit(stdout, &diag, (beet_diag_options){ false });
    return 0;
}