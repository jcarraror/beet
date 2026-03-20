#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "beet/parser/parser.h"
#include "beet/support/source.h"

static void test_simple_bind(void) {
    const char *text = "bind x = 10\n";

    beet_source_file file;
    beet_parser parser;
    beet_ast_binding binding;

    beet_source_file_init(&file);
    assert(beet_source_file_set_text_copy(&file, "test.beet", text));

    beet_parser_init(&parser, &file);

    assert(beet_parser_parse_binding(&parser, &binding));

    assert(binding.is_mutable == 0);
    assert(binding.has_type == 0);
    assert(strncmp(binding.name, "x", 1) == 0);
    assert(strncmp(binding.value_text, "10", 2) == 0);

    beet_source_file_dispose(&file);
}

static void test_mutable_bind(void) {
    const char *text = "mutable total = 0\n";

    beet_source_file file;
    beet_parser parser;
    beet_ast_binding binding;

    beet_source_file_init(&file);
    assert(beet_source_file_set_text_copy(&file, "test.beet", text));

    beet_parser_init(&parser, &file);

    assert(beet_parser_parse_binding(&parser, &binding));

    assert(binding.is_mutable == 1);
    assert(binding.has_type == 0);
    assert(strncmp(binding.name, "total", 5) == 0);

    beet_source_file_dispose(&file);
}

static void test_typed_bind(void) {
    const char *text = "bind x is Int = 5\n";

    beet_source_file file;
    beet_parser parser;
    beet_ast_binding binding;

    beet_source_file_init(&file);
    assert(beet_source_file_set_text_copy(&file, "test.beet", text));

    beet_parser_init(&parser, &file);

    assert(beet_parser_parse_binding(&parser, &binding));

    assert(binding.has_type == 1);
    assert(strncmp(binding.type_name, "Int", 3) == 0);

    beet_source_file_dispose(&file);
}

int main(void) {
    test_simple_bind();
    test_mutable_bind();
    test_typed_bind();
    return 0;
}