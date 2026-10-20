/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.2
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
// Chapter 1 - Introduction to the SNOBOL Programming Language

/**
 * 1.1 Assignment Statements
 * Bind null-terminated C string to parsing context
 * Returns a read-only C string view spanning [cstring, first null terminator)
 * @note A view *never* includes a terminating '\0' - but may contain NUL bytes.
 */
sno_view_t sno_bind(const char* cstr);

/**
 * Copy a view into buf as a C string.
 * Copies at most size - 1 chars and always terminates with '\0'.
 * A null string gives "".
 * Returns NULL, writing nothing, if size == 0 or v is a failure view.
 * @warn A NUL inside the view ends the C string early.
 */
char* sno_unbind(sno_view_t v, char* buf, size_t size);

/**
 * 1.3.1 NULL - SNOBOL's null string is a string of length zero.
 * Return a valid *empty* view "" of size 0.
 * Success, empty: begin == end.
 * @see sno_fail()
 */
sno_view_t sno_null();

/**
 * 1.3.1 NULL - test whether a view is the null string.
 * Null string: begin != NULL, begin == end. A failure view is not null.
 */
static inline bool sno_is_null(sno_view_t v) { return v.begin != NULL && v.begin == v.end; }
/**
 * 1.3.4 Input and Output of Strings
 * OUTPUT
 * Write a view to an output stream
 * Returning the number of bytes written
 * If bytes written < view size
 * the error indicator for the stream is set,
 * and errno set to indicate the error
 */
int sno_output(FILE* f, sno_view_t v);

/**
 * INPUT
 * Read one line from f into buf, replacing '\n' with '\0'.
 * Returns a view of the line, excluding the '\0'.
 * Empty line returns a null string (size 0).
 * A final line without '\n' is returned as a normal line.
 * EOF before any character, or bad args (f or buf NULL, size < 2) returns a failure view.
 * A line longer than size - 1 is truncated and the remainder is discarded.
 * @note Text mode recommended as in binary mode '\r' is kept.
 */
sno_view_t sno_input(FILE* f, char* buf, size_t size);

/**
 * 1.8 Flow Control - SNOBOL has conditional transfer on success or failure.
 * A SNOBOL expression may yield the null string *or* fail, so a view must represent both outcomes.
 * This is the *failure* view.
 * Failure: begin == NULL, end == NULL.
 */
sno_view_t sno_fail();

/**
 * 1.8 Flow Control - test whether a view represents failure.
 * Failure: begin == NULL, end == NULL.
 */
static inline bool sno_failed(sno_view_t v) { return v.begin == NULL; }

/**
 * 1.10.1 SIZE
 * Returns the length (number of characters) of the string view.
 * @warning A failure view has size 0.
 */
static inline int sno_size(sno_view_t v) { return sno_failed(v) ? 0 : (int)(v.end - v.begin); }

/**
 * 1.11 TRIM
 * Removes trailing whitespace as defined by the POSIX C locale: ' ' '\f' '\n' '\r' '\t' '\v'
 * @see sno_constants.h
 */
sno_view_t sno_trim(sno_view_t v);

// Chapter 2 - Pattern Matching

/**
 * 2.10 ANY
 * Match *one* character of the subject (u) that DOES appear in the target set (v).
 * @note The order and duplicates in the set are irrelevant.
 * @warn Greenbook: arguement must be nonnull else error program stop
 *
 * On success modifies the subject - cursor begin advances by one.
 * Returns true:
 * + The subject is non-empty and its *first* character is in the set.
 *
 * On failure leaves the subject unchanged.
 * Returns false:
 *  + first character not in set.
 *  + subject is empty ie cursor at end.
 *  + subject pointer is NULL.
 *  + The subject or set is a failure view.
 */
bool sno_any(sno_view_t* u, sno_view_t v);

/**
 * 2.10 NOTANY
 * As sno_any, but matches *one* character that does NOT appear in set (v).
 */
bool sno_notany(sno_view_t* u, sno_view_t v);

// Chapter 3 - Primitive Functions, Predicates, and Operations

/**
 * 3.3.1 IDENT
 * For the function call sno_ident(u, v) to succeed or for sno_differ(u, v) to fail,
 * the values of the arguments, u and v, must be truly identical as identity by content.
 * DIFFER is the complement of IDENT, except that both fail returning false when either argument is a failure view.
 */
bool sno_ident(sno_view_t u, sno_view_t v);

bool sno_differ(sno_view_t u, sno_view_t v);



#endif
