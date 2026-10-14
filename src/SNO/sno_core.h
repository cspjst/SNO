/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.1
 *
 * @brief SNOBOL4 Pattern Matching Library for C - Core Functions
 *
 * C function implementation of the SNOBOL4 pattern matching semantics as defined in 'The SNOBOL4 Programming Language' 2nd edition,
 * Griswold, Poage & Polonsky, 1971 (aka "The Green Book").
 */
#ifndef SNO_CORE_H
#define SNO_CORE_H

#include "sno_types.h"

#include <stdbool.h>
#include <stdio.h>

/**
 * Test if a char is in a set of chars?
 */
bool sno_char_in_set(char c, sno_view_t charset);

/**
 * Test if a char is in a null terminated string?
 */
bool sno_char_in_cstr(char c, const char* charset);

// Greenbook Sections:

/**
 * 1.3 Assignment
 * Bind null-terminated C string to parsing context
 * Returns a view spanning [cstring, first null terminator)
 */
sno_view_t sno_bind(const char* cstr);

/**
 * 1.3.1 NULL
 * Make a null view "" size 0
 */
sno_view_t sno_null();

/**
 * 1.3.4 Input and Output of Strings
 * Write a view to an output stream
 */
bool sno_output(sno_view_t subject, FILE* stream);

/**
 * 1.3.4 Input and Output of Strings
 * Read input into a cstr buffer and bind it to a view
 */
sno_view_t sno_input(char* buffer, FILE* stream);

/**
 * 1.11 TRIM remove trailing whitespace as defined in sno_constants.h
 */
bool sno_trim(sno_view_t subject);



#endif
