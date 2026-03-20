#ifndef BEET_DIAG_DIAG_H
#define BEET_DIAG_DIAG_H

#include <stdbool.h>
#include <stdio.h>

#include "beet/support/source.h"

typedef enum beet_diag_level {
  BEET_DIAG_NOTE = 0,
  BEET_DIAG_WARNING = 1,
  BEET_DIAG_ERROR = 2
} beet_diag_level; // Represents the severity level of a diagnostic message

typedef struct beet_diag {
  beet_diag_level level;
  const beet_source_file *file;
  beet_source_span span;
  const char *message;
  const char *note;
} beet_diag; // Represents a diagnostic message, including its severity level,
             // source location, and message text

typedef struct beet_diag_options {
  bool use_color;
} beet_diag_options; // Options for controlling the formatting of diagnostic
                     // messages

void beet_diag_emit(
    FILE *stream, const beet_diag *diag,
    beet_diag_options
        options); // Emits a diagnostic message to the specified output stream,
                  // formatted according to the provided options

#endif