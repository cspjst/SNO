/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.4
 *
 * @brief SNOBOL4 Pattern Matching Library for C - Core Functions
 *
 * C function implementation of the SNOBOL4 pattern matching semantics as defined in 'The SNOBOL4 Programming Language' 2nd edition,
 * Griswold, Poage & Polonsky, 1971 (aka "The Green Book").
 *
 * Failure means the text doesn't match: functions return sno_fail() or false, and failure views propagate.
 * A precondition (@pre) is a programmer error: it raises the Green Book error named, via sno_error() (Green Book 10.2.2).
 * @see sno_error.h
 */
#ifndef SNO_CORE_H
#define SNO_CORE_H

#include "sno_types.h"

#include <stdbool.h>
#include <stdio.h>

// Chapter 0 - C Support (no Green Book equivalent)

// 0.1 Character Sets

/**
 * 0.1 C CHAR IN SET
 * Test if a char is in a set of chars.
 *
 * Returns true:
 *  + c appears in the view v.
 * Returns false:
 *  + c does not appear in v.
 *  + v is the null string or a failure view.
 *
 * @note    A view may contain NUL, so '\0' can be a member.
 * @see     sno_char_in_cstr()
 */
bool sno_char_in_set(char c, sno_view_t v);

/**
 * 0.1 C CHAR IN CSTR
 * Test if a char is in a null terminated string.
 *
 * @pre charset != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *
 * Returns true:
 *  + c appears in charset before its terminator.
 * Returns false:
 *  + c does not appear in charset.
 *  + c is '\0' - the terminator is not a member.
 *
 * @see     sno_char_in_set()
 */
bool sno_char_in_cstr(char c, const char* charset);

// Greenbook Sections:
// Chapter 1 - Introduction to the SNOBOL Programming Language

// 1.1 Assignment Statements

/**
 * 1.1 C BIND
 * Bind a null-terminated C string to parsing context.
 *
 * Success: returns a read-only C string view spanning [cstring, first null terminator).
 *  + "" gives a null string.
 * Failure: returns sno_fail().
 *  + cstr is NULL - a bridge from C functions that return NULL for "nothing".
 *
 * @note    A view *never* includes a terminating '\0' - but may contain NUL bytes.
 * @see     sno_unbind()
 */
sno_view_t sno_bind(const char* cstr);

/**
 * 1.1 C UNBIND
 * Unbind the view - copy a view into buf as a C string.
 *
 * @pre     buf != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *
 * Success: returns buf; copies at most size - 1 chars and always terminates with '\0'.
 *  + A null string gives "".
 * Failure: returns NULL, writing nothing.
 *  + size == 0.
 *  + v is a failure view.
 *
 * @warning A NUL inside the view ends the C string early.
 * @note    Truncated if sno_size(v) > size - 1
 * @see     sno_bind()
 */
char* sno_unbind(sno_view_t v, char* buf, size_t size);

// 1.3 Strings

/**
 * 1.3.1 C NULL
 * Make a null view - SNOBOL's null string is a string of length zero.
 *
 * Success: returns a valid *empty* view "" of size 0: begin == end.
 *
 * @see     sno_fail(), sno_is_null()
 */
sno_view_t sno_null();

/**
 * 1.3.1 C IS NULL
 * Test whether a view is the null string.
 *
 * Returns true:
 *  + begin != NULL, begin == end.
 * Returns false:
 *  + v is non-empty.
 *  + v is a failure view - a failure view is not null.
 *
 * @see     sno_null(), sno_failed()
 */
static inline bool sno_is_null(sno_view_t v) { return v.begin != NULL && v.begin == v.end; }

/**
 * 1.3.4 SNOBOL OUTPUT
 * Write a view to an output stream; no newline is added.
 *
 * @pre     f != NULL, else error 12 ILLEGAL I/O UNIT
 *
 * Success: returns the number of bytes written.
 *  + A null string or failure view writes nothing and returns 0.
 * Failure: returns fewer bytes than the view's size.
 *  + the error indicator for the stream is set (test with ferror).
 *
 * @see     sno_input()
 */
int sno_output(FILE* f, sno_view_t v);

/**
 * 1.3.4 SNOBOL INPUT
 * Read one line from f into buf as a view.
 * '\n' is not stored; buf is terminated with '\0', which the view excludes.
 *
 * @pre     f != NULL, else error 12 ILLEGAL I/O UNIT
 * @pre     buf != NULL and size >= 2, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 * @pre     no read error, else error 11 READING ERROR
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
 * @see     sno_output(), sno_trim()
 */
sno_view_t sno_input(FILE* f, char* buf, size_t size);

// 1.8 Flow Control

/**
 * 1.8 C FAIL
 * Make a failure view.
 * SNOBOL has conditional transfer on success or failure.
 * A SNOBOL expression may yield the null string *or* fail, so a view must represent both outcomes.
 *
 * Success: returns the *failure* view: begin == NULL, end == NULL.
 *
 * @see     sno_failed(), sno_null()
 */
sno_view_t sno_fail();

/**
 * 1.8 C FAILED
 * Test whether a view represents failure.
 *
 * Returns true:
 *  + begin == NULL - including a malformed {NULL, p}, which is unsafe to read.
 * Returns false:
 *  + any readable view, including the null string.
 *
 * @see     sno_fail(), sno_is_null()
 */
static inline bool sno_failed(sno_view_t v) { return v.begin == NULL; }

// 1.10 Functions

/**
 * 1.10.1 SNOBOL SIZE(string)
 * Returns the length (number of characters) of the string view.
 *
 * @warning A failure view has size 0 - use sno_failed() to tell it from the null string.
 */
static inline int sno_size(sno_view_t v) { return sno_failed(v) ? 0 : (int)(v.end - v.begin); }

/**
 * 1.11 SNOBOL TRIM(string)
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
 * 2.8 SNOBOL LEN(length)
 * Match exactly n characters of the subject (u), whatever they are.
 *
 * @pre     u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 * @pre     n >= 0, else error 14 NEGATIVE NUMBER IN ILLEGAL CONTEXT
 *
 * Success: returns the matched view [old cursor, old cursor + n); modifies the
 *          subject - cursor begin advances by exactly n.
 *  + n == 0: returns a null string at the cursor; the subject is unchanged.
 * Failure: returns sno_fail(); leaves the subject unchanged.
 *  + fewer than n characters remain.
 *  + The subject is a failure view.
 *
 * @note    Content-agnostic: matches any characters, including NUL.
 * @see     sno_rem(), sno_tab()
 */
sno_view_t sno_len(sno_view_t* u, int n);

/**
 * 2.10 SNOBOL ANY(string)
 * Match *one* character of the subject (u) that DOES appear in the target set (v).
 *
 * @pre     u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 * @pre     v is not the null string, else error 4 NULL STRING IN ILLEGAL CONTEXT
 *
 * Success: returns the one-character matched view and modifies the subject's cursor ie u.begin advances by one.
 *  + The subject is non-empty and its *first* character is in the set.
 * Failure: returns sno_fail(); leaves the subject unchanged.
 *  + first character not in set.
 *  + subject is empty ie cursor at end.
 *  + The subject or set is a failure view.
 *
 * @note    The order and duplicates in the set are irrelevant.
 * @see     sno_notany(), sno_span()
 */
sno_view_t sno_any(sno_view_t* u, sno_view_t v);

/**
 * 2.10 SNOBOL NOTANY(string)
 * Match *one* character of the subject (u) that does NOT appear in set (v).
 *
 * @pre     u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 * @pre     v is not the null string, else error 4 NULL STRING IN ILLEGAL CONTEXT
 *
 * Success: returns the one-character matched view and modifies the subject's cursor ie u.begin advances by one.
 *  + The subject is non-empty and its *first* character is not in the set.
 * Failure: returns sno_fail(); leaves the subject unchanged.
 *  + first character in set.
 *  + subject is empty ie cursor at end - there is no character to match.
 *  + The subject or set is a failure view.
 *
 * @note    Not simply the negation of sno_any(): at end of subject both fail.
 * @see     sno_any(), sno_break()
 */
sno_view_t sno_notany(sno_view_t* u, sno_view_t v);

// Chapter 3 - Primitive Functions, Predicates, and Operations

/**
 * 3.3.1 SNOBOL IDENT(X,Y)
 * Test whether u and v are truly identical, by content.
 *
 * Returns true:
 *  + u and v have the same size and the same characters.
 *  + both are null strings (every null string is identical).
 * Returns false:
 *  + they differ in size or in any character.
 *  + either is a failure view.
 *
 * @note    SNOBOL interns strings, so it compares pointers; views are not
 *          interned, so identity must be by content.
 * @see     sno_differ()
 */
bool sno_ident(sno_view_t u, sno_view_t v);

/**
 * 3.3.1 SNOBOL DIFFER(X,Y)
 * The complement of IDENT.
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
