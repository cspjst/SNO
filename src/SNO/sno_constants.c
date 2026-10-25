#include "sno_constants.h"

// sno_constants.c
const char SNO_POSIX_WHITESPACE[] = " \f\n\r\t\v";
const sno_view_t SNO_SPACE      = { SNO_POSIX_WHITESPACE,     SNO_POSIX_WHITESPACE + 1 };
const sno_view_t SNO_NEWLINE    = { SNO_POSIX_WHITESPACE + 2, SNO_POSIX_WHITESPACE + 3 };
const sno_view_t SNO_TAB        = { SNO_POSIX_WHITESPACE + 4, SNO_POSIX_WHITESPACE + 5 };
const sno_view_t SNO_WHITESPACE = { SNO_POSIX_WHITESPACE,     SNO_POSIX_WHITESPACE + 6 };
