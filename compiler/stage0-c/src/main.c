#include <stdio.h>
#include <stdlib.h>

#include "beet/compiler/api.h"
#include "beet/lexer/lexer.h"
#include "beet/lexer/token.h"
#include "beet/support/source.h"

static void beet_dump_tokens(const beet_source_file *file) {
    beet_lexer lexer;
    beet_token token;

    beet_lexer_init(&lexer, file);

    do {
        token = beet_lexer_next(&lexer);
        printf(
            "%s [%u:%u..%u:%u] \"%.*s\"\n",
            beet_token_kind_name(token.kind),
            token.span.start.line,
            token.span.start.column,
            token.span.end.line,
            token.span.end.column,
            (int)token.lexeme_len,
            token.lexeme
        );
    } while (token.kind != BEET_TOKEN_EOF);
}

int main(void) {
    const char *text =
        "type Point = structure {\n"
        "    x is Int\n"
        "    y is Int\n"
        "}\n"
        "\n"
        "function main() returns Int {\n"
        "    bind point = Point(x = 3, y = 4)\n"
        "    mutable total = 0\n"
        "    return 0\n"
        "}\n";

    beet_source_file file;

    beet_source_file_init(&file);

    if (!beet_source_file_set_text_copy(&file, "demo.beet", text)) {
        fputs("failed to initialize source file\n", stderr);
        return 1;
    }

    puts(BEET_COMPILER_NAME ": stage0 compiler bootstrap");
    beet_dump_tokens(&file);

    beet_source_file_dispose(&file);
    return 0;
}