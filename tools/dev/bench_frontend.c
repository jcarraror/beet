#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "beet/lexer/token.h"
#include "beet/parser/parser.h"
#include "beet/resolve/scope.h"
#include "beet/semantics/registry.h"
#include "beet/support/source.h"
#include "beet/types/check.h"

#define BEET_BENCH_MAX_SOURCE_BYTES 32768
#define BEET_BENCH_MAX_TYPE_DECLS 8
#define BEET_BENCH_MAX_FUNCTIONS 8
#define BEET_BENCH_PARSE_WARMUP_DEFAULT 2U
#define BEET_BENCH_PARSE_ITERS_DEFAULT 16U
#define BEET_BENCH_RESOLVE_WARMUP_DEFAULT 16U
#define BEET_BENCH_RESOLVE_ITERS_DEFAULT 512U
#define BEET_BENCH_TYPE_WARMUP_DEFAULT 8U
#define BEET_BENCH_TYPE_ITERS_DEFAULT 256U
#define BEET_BENCH_SAMPLES_DEFAULT 5U
#define BEET_BENCH_TARGET_MS_DEFAULT 20.0
#define BEET_BENCH_MAX_ITERS 1000000U

typedef struct beet_bench_fixture {
  beet_source_file file;
  beet_parser parser;
  beet_ast_type_decl type_decls[BEET_BENCH_MAX_TYPE_DECLS];
  beet_ast_function functions[BEET_BENCH_MAX_FUNCTIONS];
  beet_decl_registry registry;
  size_t type_decl_count;
  size_t function_count;
} beet_bench_fixture;

typedef struct beet_bench_phase_summary {
  const char *label;
  size_t bytes_per_iter;
  size_t iterations;
  size_t samples;
  double min_seconds;
  double median_seconds;
  double max_seconds;
  double mib_per_second;
} beet_bench_phase_summary;

static double beet_bench_now_seconds(void) {
  struct timespec ts;

  assert(timespec_get(&ts, TIME_UTC) == TIME_UTC);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
}

static size_t beet_bench_env_size(const char *name, size_t default_value) {
  const char *value;
  char *end;
  unsigned long parsed;

  assert(name != NULL);

  value = getenv(name);
  if (value == NULL || value[0] == '\0') {
    return default_value;
  }

  parsed = strtoul(value, &end, 10);
  if (end == value || *end != '\0' || parsed == 0UL) {
    return default_value;
  }

  return (size_t)parsed;
}

static double beet_bench_env_double(const char *name, double default_value) {
  const char *value;
  char *end;
  double parsed;

  assert(name != NULL);

  value = getenv(name);
  if (value == NULL || value[0] == '\0') {
    return default_value;
  }

  parsed = strtod(value, &end);
  if (end == value || *end != '\0' || parsed <= 0.0) {
    return default_value;
  }

  return parsed;
}

static int beet_bench_append(char *buffer, size_t capacity, size_t *offset,
                             const char *format, ...) {
  va_list args;
  int written;

  assert(buffer != NULL);
  assert(offset != NULL);
  assert(format != NULL);

  if (*offset >= capacity) {
    return 0;
  }

  va_start(args, format);
  written = vsnprintf(buffer + *offset, capacity - *offset, format, args);
  va_end(args);

  if (written < 0 || (size_t)written >= capacity - *offset) {
    return 0;
  }

  *offset += (size_t)written;
  return 1;
}

static int beet_bench_build_source(char *buffer, size_t capacity,
                                   size_t *out_len) {
  size_t offset;
  size_t i;

  assert(buffer != NULL);
  assert(out_len != NULL);

  offset = 0U;

  if (!beet_bench_append(buffer, capacity, &offset,
                         "type Pair = structure {\n"
                         "    left is Int\n"
                         "    right is Int\n"
                         "}\n"
                         "\n"
                         "type OptionInt = choice {\n"
                         "    none\n"
                         "    some(Int)\n"
                         "}\n"
                         "\n"
                         "function helper(x is Int) returns Int {\n"
                         "    mutable total = x\n")) {
    return 0;
  }

  for (i = 0U; i < 24U; ++i) {
    if (!beet_bench_append(buffer, capacity, &offset,
                           "    total = total + 1\n")) {
      return 0;
    }
  }

  if (!beet_bench_append(
          buffer, capacity, &offset,
          "    if total > 10 {\n"
          "        total = total - 1\n"
          "    } else {\n"
          "        total = total + 2\n"
          "    }\n"
          "    return total\n"
          "}\n"
          "\n"
          "function main() returns Int {\n"
          "    mutable total = 0\n"
          "    bind pair is Pair = Pair(left = 1, right = 2)\n"
          "    bind maybe_value is OptionInt = OptionInt.some(pair.left)\n"
          "    match maybe_value {\n"
          "        case none {\n"
          "            return 0\n"
          "        }\n"
          "        case some(item) {\n"
          "            total = helper(item)\n"
          "        }\n"
          "    }\n")) {
    return 0;
  }

  for (i = 0U; i < 24U; ++i) {
    if (!beet_bench_append(buffer, capacity, &offset,
                           "    total = total + 1\n")) {
      return 0;
    }
  }

  if (!beet_bench_append(buffer, capacity, &offset,
                         "    total = helper(total)\n"
                         "    if total > 100 {\n"
                         "        return total\n"
                         "    } else {\n"
                         "        return total + pair.right\n"
                         "    }\n"
                         "}\n")) {
    return 0;
  }

  *out_len = offset;
  return 1;
}

static int beet_bench_parse_fixture(beet_bench_fixture *fixture,
                                    const char *source_text) {
  assert(fixture != NULL);
  assert(source_text != NULL);

  memset(fixture, 0, sizeof(*fixture));
  beet_source_file_init(&fixture->file);
  if (!beet_source_file_set_text_copy(&fixture->file, "bench_frontend.beet",
                                      source_text)) {
    return 0;
  }

  beet_parser_init(&fixture->parser, &fixture->file);

  while (fixture->parser.current.kind == BEET_TOKEN_KW_TYPE) {
    if (fixture->type_decl_count >= BEET_BENCH_MAX_TYPE_DECLS) {
      beet_source_file_dispose(&fixture->file);
      return 0;
    }

    if (!beet_parser_parse_type_decl(
            &fixture->parser, &fixture->type_decls[fixture->type_decl_count])) {
      fprintf(stderr, "benchmark parse: failed type decl at token %s\n",
              beet_token_kind_name(fixture->parser.current.kind));
      beet_source_file_dispose(&fixture->file);
      return 0;
    }

    fixture->type_decl_count += 1U;
  }

  while (fixture->parser.current.kind == BEET_TOKEN_KW_FUNCTION) {
    if (fixture->function_count >= BEET_BENCH_MAX_FUNCTIONS) {
      beet_source_file_dispose(&fixture->file);
      return 0;
    }

    if (!beet_parser_parse_function(
            &fixture->parser, &fixture->functions[fixture->function_count])) {
      fprintf(stderr, "benchmark parse: failed function %zu at token %s\n",
              fixture->function_count,
              beet_token_kind_name(fixture->parser.current.kind));
      beet_source_file_dispose(&fixture->file);
      return 0;
    }

    fixture->function_count += 1U;
  }

  if (fixture->parser.current.kind != BEET_TOKEN_EOF) {
    fprintf(stderr, "benchmark parse: unexpected top-level token %s\n",
            beet_token_kind_name(fixture->parser.current.kind));
    beet_source_file_dispose(&fixture->file);
    return 0;
  }

  beet_decl_registry_init(&fixture->registry, fixture->type_decls,
                          fixture->type_decl_count, fixture->functions,
                          fixture->function_count);
  return 1;
}

static void beet_bench_dispose_fixture(beet_bench_fixture *fixture) {
  assert(fixture != NULL);
  beet_source_file_dispose(&fixture->file);
}

static int beet_bench_run_parse(const char *source_text, size_t warmup_iters,
                                size_t measure_iters, double *out_seconds) {
  size_t i;
  double start;

  assert(source_text != NULL);
  assert(out_seconds != NULL);

  for (i = 0U; i < warmup_iters; ++i) {
    beet_bench_fixture fixture;

    if (!beet_bench_parse_fixture(&fixture, source_text)) {
      return 0;
    }
    beet_bench_dispose_fixture(&fixture);
  }

  start = beet_bench_now_seconds();
  for (i = 0U; i < measure_iters; ++i) {
    beet_bench_fixture fixture;

    if (!beet_bench_parse_fixture(&fixture, source_text)) {
      return 0;
    }
    beet_bench_dispose_fixture(&fixture);
  }
  *out_seconds = beet_bench_now_seconds() - start;
  return 1;
}

static int beet_bench_run_resolve(const char *source_text, size_t warmup_iters,
                                  size_t measure_iters, double *out_seconds) {
  beet_bench_fixture fixture;
  size_t i;
  size_t function_index;
  double start;

  assert(source_text != NULL);
  assert(out_seconds != NULL);

  if (!beet_bench_parse_fixture(&fixture, source_text)) {
    return 0;
  }

  for (i = 0U; i < warmup_iters; ++i) {
    for (function_index = 0U; function_index < fixture.function_count;
         ++function_index) {
      if (!beet_resolve_function_with_registry(
              &fixture.functions[function_index], &fixture.registry)) {
        beet_bench_dispose_fixture(&fixture);
        return 0;
      }
    }
  }

  start = beet_bench_now_seconds();
  for (i = 0U; i < measure_iters; ++i) {
    for (function_index = 0U; function_index < fixture.function_count;
         ++function_index) {
      if (!beet_resolve_function_with_registry(
              &fixture.functions[function_index], &fixture.registry)) {
        beet_bench_dispose_fixture(&fixture);
        return 0;
      }
    }
  }
  *out_seconds = beet_bench_now_seconds() - start;
  beet_bench_dispose_fixture(&fixture);
  return 1;
}

static int beet_bench_run_type_check(const char *source_text,
                                     size_t warmup_iters, size_t measure_iters,
                                     double *out_seconds) {
  beet_bench_fixture fixture;
  size_t i;
  size_t function_index;
  double start;

  assert(source_text != NULL);
  assert(out_seconds != NULL);

  if (!beet_bench_parse_fixture(&fixture, source_text)) {
    return 0;
  }

  for (function_index = 0U; function_index < fixture.function_count;
       ++function_index) {
    if (!beet_resolve_function_with_registry(&fixture.functions[function_index],
                                             &fixture.registry)) {
      beet_bench_dispose_fixture(&fixture);
      return 0;
    }
  }

  for (i = 0U; i < warmup_iters; ++i) {
    if (!beet_type_check_type_decls_with_registry(&fixture.registry)) {
      beet_bench_dispose_fixture(&fixture);
      return 0;
    }

    for (function_index = 0U; function_index < fixture.function_count;
         ++function_index) {
      if (!beet_type_check_function_signature_with_registry(
              &fixture.functions[function_index], &fixture.registry) ||
          !beet_type_check_function_body_with_registry(
              &fixture.functions[function_index], &fixture.registry)) {
        beet_bench_dispose_fixture(&fixture);
        return 0;
      }
    }
  }

  start = beet_bench_now_seconds();
  for (i = 0U; i < measure_iters; ++i) {
    if (!beet_type_check_type_decls_with_registry(&fixture.registry)) {
      beet_bench_dispose_fixture(&fixture);
      return 0;
    }

    for (function_index = 0U; function_index < fixture.function_count;
         ++function_index) {
      if (!beet_type_check_function_signature_with_registry(
              &fixture.functions[function_index], &fixture.registry) ||
          !beet_type_check_function_body_with_registry(
              &fixture.functions[function_index], &fixture.registry)) {
        beet_bench_dispose_fixture(&fixture);
        return 0;
      }
    }
  }

  *out_seconds = beet_bench_now_seconds() - start;
  beet_bench_dispose_fixture(&fixture);
  return 1;
}

static void beet_bench_sort_doubles(double *values, size_t count) {
  size_t i;
  size_t j;

  assert(values != NULL || count == 0U);

  for (i = 1U; i < count; ++i) {
    double value = values[i];

    j = i;
    while (j > 0U && values[j - 1U] > value) {
      values[j] = values[j - 1U];
      j -= 1U;
    }
    values[j] = value;
  }
}

static double beet_bench_compute_mib_per_second(size_t bytes_per_iter,
                                                size_t iterations,
                                                double seconds) {
  if (seconds <= 0.0) {
    seconds = 0.000001;
  }

  return (((double)bytes_per_iter * (double)iterations) / (1024.0 * 1024.0)) /
         seconds;
}

static size_t beet_bench_scale_iterations(size_t current_iters,
                                          double current_seconds,
                                          double target_seconds) {
  double scaled;

  assert(current_iters > 0U);

  if (current_seconds <= 0.0 || current_seconds >= target_seconds) {
    return current_iters;
  }

  scaled = ((double)current_iters * target_seconds) / current_seconds;
  if (scaled < (double)(current_iters + 1U)) {
    scaled = (double)(current_iters + 1U);
  }
  if (scaled > (double)BEET_BENCH_MAX_ITERS) {
    return BEET_BENCH_MAX_ITERS;
  }

  return (size_t)scaled;
}

static int beet_bench_collect_samples(
    const char *source_text, size_t warmup_iters, size_t *io_measure_iters,
    size_t sample_count, double target_seconds,
    int (*run_phase)(const char *, size_t, size_t, double *),
    beet_bench_phase_summary *out_summary) {
  double samples[32];
  double pilot_seconds;
  size_t sample_index;
  size_t measure_iters;

  assert(source_text != NULL);
  assert(io_measure_iters != NULL);
  assert(run_phase != NULL);
  assert(out_summary != NULL);
  assert(sample_count > 0U && sample_count <= 32U);

  measure_iters = *io_measure_iters;
  if (!run_phase(source_text, warmup_iters, measure_iters, &pilot_seconds)) {
    return 0;
  }

  measure_iters =
      beet_bench_scale_iterations(measure_iters, pilot_seconds, target_seconds);
  *io_measure_iters = measure_iters;

  for (sample_index = 0U; sample_index < sample_count; ++sample_index) {
    if (!run_phase(source_text, warmup_iters, measure_iters,
                   &samples[sample_index])) {
      return 0;
    }
  }

  beet_bench_sort_doubles(samples, sample_count);
  out_summary->iterations = measure_iters;
  out_summary->samples = sample_count;
  out_summary->min_seconds = samples[0];
  out_summary->median_seconds = samples[sample_count / 2U];
  out_summary->max_seconds = samples[sample_count - 1U];
  out_summary->mib_per_second = beet_bench_compute_mib_per_second(
      out_summary->bytes_per_iter, measure_iters, out_summary->median_seconds);
  return 1;
}

static void
beet_bench_print_text_result(const beet_bench_phase_summary *summary) {
  assert(summary != NULL);

  printf("%-10s %7zu iters  %7zu samples  %8.3f ms  %8.3f..%-8.3f ms  %8.2f "
         "MiB/s\n",
         summary->label, summary->iterations, summary->samples,
         summary->median_seconds * 1000.0, summary->min_seconds * 1000.0,
         summary->max_seconds * 1000.0, summary->mib_per_second);
}

static const beet_bench_phase_summary *
beet_bench_find_hotspot(const beet_bench_phase_summary *summaries,
                        size_t summary_count) {
  const beet_bench_phase_summary *slowest;
  size_t i;

  assert(summaries != NULL);
  assert(summary_count > 0U);

  slowest = &summaries[0];
  for (i = 1U; i < summary_count; ++i) {
    if (summaries[i].mib_per_second < slowest->mib_per_second) {
      slowest = &summaries[i];
    }
  }

  return slowest;
}

static void beet_bench_print_json(const beet_bench_phase_summary *summaries,
                                  size_t summary_count, size_t source_len,
                                  double target_ms) {
  const beet_bench_phase_summary *hotspot;
  size_t i;

  assert(summaries != NULL);
  assert(summary_count > 0U);

  hotspot = beet_bench_find_hotspot(summaries, summary_count);
  printf("{\"source_bytes\":%zu,\"target_ms\":%.3f,\"hotspot\":\"%s\","
         "\"phases\":[",
         source_len, target_ms, hotspot->label);
  for (i = 0U; i < summary_count; ++i) {
    const beet_bench_phase_summary *summary = &summaries[i];

    if (i > 0U) {
      printf(",");
    }
    printf("{\"name\":\"%s\",\"iterations\":%zu,\"samples\":%zu,"
           "\"median_ms\":%.3f,\"min_ms\":%.3f,\"max_ms\":%.3f,"
           "\"mib_per_s\":%.2f}",
           summary->label, summary->iterations, summary->samples,
           summary->median_seconds * 1000.0, summary->min_seconds * 1000.0,
           summary->max_seconds * 1000.0, summary->mib_per_second);
  }
  printf("]}\n");
}

int main(int argc, char **argv) {
  char source_text[BEET_BENCH_MAX_SOURCE_BYTES];
  beet_bench_phase_summary summaries[3];
  const beet_bench_phase_summary *hotspot;
  size_t source_len;
  size_t parse_warmup;
  size_t parse_iters;
  size_t resolve_warmup;
  size_t resolve_iters;
  size_t type_warmup;
  size_t type_iters;
  size_t sample_count;
  double target_ms;
  int json_output;

  json_output = (argc > 1 && strcmp(argv[1], "--json") == 0);

  if (!beet_bench_build_source(source_text, sizeof(source_text), &source_len)) {
    fprintf(stderr, "error: failed to build benchmark source\n");
    return 1;
  }

  parse_warmup = beet_bench_env_size("BEET_BENCH_PARSE_WARMUP",
                                     BEET_BENCH_PARSE_WARMUP_DEFAULT);
  parse_iters = beet_bench_env_size("BEET_BENCH_PARSE_ITERS",
                                    BEET_BENCH_PARSE_ITERS_DEFAULT);
  resolve_warmup = beet_bench_env_size("BEET_BENCH_RESOLVE_WARMUP",
                                       BEET_BENCH_RESOLVE_WARMUP_DEFAULT);
  resolve_iters = beet_bench_env_size("BEET_BENCH_RESOLVE_ITERS",
                                      BEET_BENCH_RESOLVE_ITERS_DEFAULT);
  type_warmup = beet_bench_env_size("BEET_BENCH_TYPE_WARMUP",
                                    BEET_BENCH_TYPE_WARMUP_DEFAULT);
  type_iters = beet_bench_env_size("BEET_BENCH_TYPE_ITERS",
                                   BEET_BENCH_TYPE_ITERS_DEFAULT);
  sample_count =
      beet_bench_env_size("BEET_BENCH_SAMPLES", BEET_BENCH_SAMPLES_DEFAULT);
  if (sample_count > 32U) {
    sample_count = 32U;
  }
  target_ms = beet_bench_env_double("BEET_BENCH_TARGET_MS",
                                    BEET_BENCH_TARGET_MS_DEFAULT);

  summaries[0].label = "parse";
  summaries[0].bytes_per_iter = source_len;
  if (!beet_bench_collect_samples(source_text, parse_warmup, &parse_iters,
                                  sample_count, target_ms / 1000.0,
                                  beet_bench_run_parse, &summaries[0])) {
    fprintf(stderr, "error: parse benchmark failed\n");
    return 1;
  }

  summaries[1].label = "resolve";
  summaries[1].bytes_per_iter = source_len;
  if (!beet_bench_collect_samples(source_text, resolve_warmup, &resolve_iters,
                                  sample_count, target_ms / 1000.0,
                                  beet_bench_run_resolve, &summaries[1])) {
    fprintf(stderr, "error: resolve benchmark failed\n");
    return 1;
  }

  summaries[2].label = "type-check";
  summaries[2].bytes_per_iter = source_len;
  if (!beet_bench_collect_samples(source_text, type_warmup, &type_iters,
                                  sample_count, target_ms / 1000.0,
                                  beet_bench_run_type_check, &summaries[2])) {
    fprintf(stderr, "error: type-check benchmark failed\n");
    return 1;
  }

  if (json_output) {
    beet_bench_print_json(summaries, 3U, source_len, target_ms);
    return 0;
  }

  hotspot = beet_bench_find_hotspot(summaries, 3U);
  printf("beet frontend benchmark\n");
  printf("source bytes: %zu\n", source_len);
  printf("target per phase: %.1f ms\n", target_ms);
  beet_bench_print_text_result(&summaries[0]);
  beet_bench_print_text_result(&summaries[1]);
  beet_bench_print_text_result(&summaries[2]);
  printf("hotspot: %s\n", hotspot->label);
  printf("compare median MiB/s across commits; higher is better\n");

  return 0;
}
