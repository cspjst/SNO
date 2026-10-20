/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.3
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
 * CHAR IN SET Test if a char is in a set of chars.
 * Returns true:
 *  + c appears in the view v.
 * Returns false:
 *  + c does not appear in v.
 *  + v is the null string or a failure view.
 * @note    A view may contain NUL, so '\0' can be a member.
 */
bool sno_char_in_set(char c, sno_view_t v);

/**
 * CHAR IN CSTR Test if a char is in a null terminated string.
 * @pre     charset != NULL
 * Returns true:
 *  + c appears in charset before its terminator.
 * Returns false:
 *  + c does not appear in charset.
 *  + c is '\0' - the terminator is not a member.
 */
bool sno_char_in_cstr(char c, const char* charset);

// Greenbook Sections:
// Chapter 1 - Introduction to the SNOBOL Programming Language

/**
 * 1.1 Assignment Statements
 * BIND null-terminated C string to parsing context.
 *
 * Success: returns a read-only C string view spanning [cstring, first null terminator).
 *  + "" gives a null string.
 * Failure: returns sno_fail().
 *  + cstr is NULL - a bridge from C functions that return NULL for "nothing".
 *
 * @note    A view *never* includes a terminating '\0' - but may contain NUL bytes.
 */
sno_view_t sno_bind(const char* cstr);

/**
 * UNBIND Copy a view into buf as a C string.
 * @pre     buf != NULL
 *
 * Success: returns buf; copies at most size - 1 chars and always terminates with '\0'.
 *  + A null string gives "".
 * Failure: returns NULL, writing nothing.
 *  + size == 0.
 *  + v is a failure view.
 *
 * @note    A NUL inside the view ends the C string early.
 * @note    Truncated if sno_size(v) > size - 1; check before calling if it matters.
 */
char* sno_unbind(sno_view_t v, char* buf, size_t size);

/**
 * 1.3.1 NULL SNOBOL's null string is a string of length zero.
 *
 * Success: returns a valid *empty* view "" of size 0: begin == end.
 *
 * @see     sno_fail(), sno_is_null()
 */
sno_view_t sno_null();

/**
 * 1.3.1 NULL Test whether a view is the null string.
 *
 * Returns true:
 *  + begin != NULL, begin == end.
 * Returns false:
 *  + v is non-empty.
 *  + v is a failure view - a failure view is not null.
 */
static inline bool sno_is_null(sno_view_t v) { return v.begin != NULL && v.begin == v.end; }

/**
 * 1.3.4 OUTPUT Write a view to an output stream; no newline is added.
 * @pre     f != NULL (Green Book error 12, ILLEGAL I/O UNIT)
 *
 * Success: returns the number of bytes written.
 *  + A null string or failure view writes nothing and returns 0.
 * Failure: returns fewer bytes than the view's size.
 *  + the error indicator for the stream is set (test with ferror).
 */
int sno_output(FILE* f, sno_view_t v);

/**
 * 1.3.4 INPUT Read one line from f into buf as a view.
 * @note line terminating '\n' is replaced with '\0' in the target buffer.
 * @pre     f != NULL (Green Book error 12, ILLEGAL I/O UNIT)
 * @pre     buf != NULL, size >= 2
 * @pre     no read error (Green Book error 11, READING ERROR)
 *
 * Success: returns a view of the line at buf.
 *  + Empty line returns a null string (size 0).
 *  + A final line without '\n' is returned as a normal line.
 *  + A line longer than size - 1 is truncated and the remainder is discarded.
 * Failure: returns sno_fail().
 *  + EOF before any character is read.
 *
 * @note    One call reads exactly one line, so a read loop always terminates.
 * @note    Possibly truncated if sno_size(v) == size - 1.
 * @note    Text mode recommended as in binary mode '\r' is kept.
 * @see     sno_output(), sno_trim()
 */
sno_view_t sno_input(FILE* f, char* buf, size_t size);

/**
 * 1.8 Flow Control
 * SNOBOL has conditional transfer on success or failure.
 * A SNOBOL expression may yield the null string *or* fail, so a view must represent both outcomes.
 *
 * Success: returns the *failure* view: begin == NULL, end == NULL.
 *
 * @see     sno_failed(), sno_null()
 */
sno_view_t sno_fail();

/**
 * 1.8 Flow Control
 * Test whether a view represents failure.
 *
 * Returns true:
 *  + begin == NULL - including a malformed {NULL, p}, which is unsafe to read.
 * Returns false:
 *  + any readable view, including the null string.
 */
static inline bool sno_failed(sno_view_t v) { return v.begin == NULL; }

/**
 * 1.10.1 SIZE Returns the length (number of characters) of the string view.
 *
 * @note    A failure view has size 0 - use sno_failed() to tell it from the null string.
 */
static inline int sno_size(sno_view_t v) { return sno_failed(v) ? 0 : (int)(v.end - v.begin); }

/**
 * 1.11 TRIM
 * Removes trailing whitespace as defined by the POSIX C locale: ' ' '\f' '\n' '\r' '\t' '\v'
 *
 * Success: returns v with end moved back over trailing whitespace; no copy.
 *  + Leading and interior whitespace is kept.
 *  + An all-whitespace view gives the null string.
 * Failure: returns the failure view unchanged.
 *  + v is a failure view.
 *
 * @see     sno_constants.h
 */
sno_view_t sno_trim(sno_view_t v);

// Chapter 2 - Pattern Matching

/**
 * 2.10 ANY Match *one* character of the subject (u) that DOES appear in the target set (v).
 * @pre     u != NULL
 * @pre     v is not the null string (Green Book error 4, NULL STRING IN ILLEGAL CONTEXT)
 *
 * Success: returns true; modifies the subject - cursor begin advances by one.
 *  + The subject is non-empty and its *first* character is in the set.
 * Failure: returns false; leaves the subject unchanged.
 *  + first character not in set.
 *  + subject is empty ie cursor at end.
 *  + The subject or set is a failure view.
 *
 * @note    The order and duplicates in the set are irrelevant.
 * @see     sno_notany()
 */
bool sno_any(sno_view_t* u, sno_view_t v);

/**
 * 2.10 NOTANY Match *one* character of the subject (u) that does NOT appear in set (v).
 * @pre     u != NULL
 * @pre     v is not the null string (Green Book error 4, NULL STRING IN ILLEGAL CONTEXT)
 *
 * Success: returns true; modifies the subject - cursor begin advances by one.
 *  + The subject is non-empty and its *first* character is not in the set.
 * Failure: returns false; leaves the subject unchanged.
 *  + first character in set.
 *  + subject is empty ie cursor at end - there is no character to match.
 *  + The subject or set is a failure view.
 *
 * @note    Not simply !sno_any() because at end of subject both fail.
 * @see     sno_any()
 */
bool sno_notany(sno_view_t* u, sno_view_t v);

// Chapter 3 - Primitive Functions, Predicates, and Operations

/**
 * 3.3.1 IDENT Test whether u and v are truly identical, by content.
 *
 * Returns true:
 *  + u and v have the same size and the same characters.
 *  + both are null strings (every null string is identical).
 * Returns false:
 *  + they differ in size or in any character.
 *  + either is a failure view.
 *
 * @see     sno_differ()
 */
bool sno_ident(sno_view_t u, sno_view_t v);

/**
 * 3.3.1 DIFFER The complement of IDENT.
 *
 * Returns true:
 *  + u and v differ in size or in any character.
 * Returns false:
 *  + u and v are identical.
 *  + either is a failure view - in SNOBOL the call is never made.
 *
 * @note    Not simply !sno_ident() because both return false on failure.
 * @see     sno_ident()
 */
bool sno_differ(sno_view_t u, sno_view_t v);

#endif
