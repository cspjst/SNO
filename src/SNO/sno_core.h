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
bool sno_char_in_set(char c, sno_view_t v);

/**
 * Test if a char is in a null terminated string?
 */
bool sno_char_in_cstr(char c, const char* charset);

// Greenbook Sections:

/**
 * 1.3 Assignment
 * Bind null-terminated C string to parsing context
 * Returns an immutable string view spanning [cstring, first null terminator)
 */
sno_view_t sno_bind(const char* cstr);

/**
 * Unbind an immutable string view to a null terminated buffer
 * Returning pointer to first char in the buffer
 * sno_unbind(v, buf, size): copies at most size - 1 chars
 * Always terminates when size > 0
 * With size == 0 it writes nothing
 */
char* sno_unbind(sno_view_t v, char* buf, size_t size);

/**
 * 1.3.1 NULL
 * Make a null view "" size 0
 */
sno_view_t sno_null();

/**
 * 1.3.4 Input and Output of Strings
 * INPUT
 * Write a view to an output stream
 * Returning the number of bytes written
 * If bytes written < view size
 * the error indicator for the stream is set,
 * and errno set to indicate the error
 */
int sno_output(FILE* f, sno_view_t v);

/**
 * OUTPUT
 * Read input into a cstr buffer and bind it to a view
 * Returning a string view of the input buffer
 * If the string is NULL
 * the error indicator for the stream is set,
 * and errno set to indicate the error
 */
sno_view_t sno_input(FILE* f, char* buf, size_t size);

/**
 * 1.10.1 Primitive Functions
 * SIZE
 * Returns length (number of characters) of the string view
 */
inline int sno_size(sno_view_t v) { return v.end - v.begin; }

/**
 * 1.11 TRIM remove trailing whitespace as defined by the isspace()
 */
sno_view_t sno_trim(sno_view_t v);



#endif
