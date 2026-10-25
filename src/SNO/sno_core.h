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
 * Views: text is handled as views [begin, end) that never own or copy it (see sno_types.h).
 * When matching, a subject's begin is the cursor; a pattern function advances it on success
 * and leaves it unchanged on failure. Matching is anchored with no implicit backtracking:
 * functions chain by passing views, and alternatives are written by the caller.
 *
 * Return types: a function that takes a subject (sno_view_t* u) returns a view.
 * Pattern functions return the matched view; tests of the cursor (RPOS) return a null string
 * at the cursor for true and sno_fail() for false - SNOBOL's own model (Green Book ch. 3:
 * predicates return the null string or fail). Functions that take no subject
 * (IDENT, DIFFER, sno_failed, sno_is_null) return bool.
 *
 * Failure means the text doesn't match: functions return sno_fail() or false, and failure views propagate.
 * A precondition is a programmer error: it raises the Green Book error named, via sno_error(), which
 * halts with the error number as the exit code - as SNOBOL's default &ERRLIMIT = 0 terminates (Green Book 10.2.2).
 * @see sno_types.h, sno_error.h
 */
#ifndef SNO_CORE_H
#define SNO_CORE_H

#include "sno_types.h"

#include <stddef.h>
#include <stdbool.h>

/**
 * 1.8 C S_
 * Shorthand for sno_succeeded(v) - reads like SNOBOL's :S column.
 * @note The null string succeeds: S_ is SNOBOL's :S, not "matched something".
 * @see     sno_succeeded(), F_()
 */
#define S_(v) sno_succeeded(v)

/**
 * 1.8 C F_
 * Shorthand for sno_failed(v) - reads like SNOBOL's :F column.
 * @see     sno_failed(), S_()
 */
#define F_(v) sno_failed(v)

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

/**
 * 1.8 C SUCCEEDED
 * Test whether a view represents success - the complement of sno_failed().
 *
 * Returns true:
 *  + begin != NULL - any readable view, including the null string.
 * Returns false:
 *  + begin == NULL - a failure view, including a malformed {NULL, p}.
 *
 * @note
 *  + The null string succeeds: success is SNOBOL's :S, not "matched something".
 *  + Reads chains without negation - sno_succeeded(sno_len(&u, 2)) && sno_succeeded(sno_rem(&u)).
 * @see     sno_failed(), sno_fail(), sno_is_null()
 */
static inline bool sno_succeeded(sno_view_t v) { return v.begin != NULL; }

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
 * 2.9 SNOBOL BREAK(string)
 * Match up to, but not including, the first character of the subject (u) that appears in the break set (v).
 *
 * @pre
 *  + u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *  + v is not the null string, else error 4 NULL STRING IN ILLEGAL CONTEXT
 *
 * Success: returns the matched view [old cursor, break character); the cursor moves to the break character.
 *  + The cursor is at a break character: returns a null string at the cursor; the subject is unchanged.
 * Failure: returns sno_fail(); leaves the subject unchanged.
 *  + No break character before the end of the subject - "BREAK must find a break character, or it fails".
 *  + The subject is empty.
 *  + The subject or set is a failure view.
 *
 * @warning Does not consume the break character: a second BREAK on the same set matches the null string
 *          again - move past it with LEN(1), ANY or SPAN before breaking again.
 * @note
 *  + The earliest break character in the subject wins; order and duplicates in the set are irrelevant.
 *  + NUL and high-bit characters are ordinary members.
 * @see     sno_span(), sno_notany(), sno_next_lit()
 */
sno_view_t sno_break(sno_view_t* u, sno_view_t v);

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

/**
 * 2.11 SNOBOL RTAB(integer)
 * Match from the cursor up to, but not including, the nth character from the end.
 *
 * @pre     u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 * @pre     n >= 0, else error 14 NEGATIVE NUMBER IN ILLEGAL CONTEXT
 *
 * Success: returns the matched view [old cursor, u->end - n)
 * modifies the subject - cursor begin moves to u->end - n.
 *  + n == number of characters remaining: returns a null string at the
 *    cursor; the subject is unchanged.
 *  + n == 0: matches the rest of the subject (REM).
 * Failure: returns sno_fail(); leaves the subject unchanged.
 *  + u->end - n is left of the cursor - fewer than n characters remain.
 *  + The subject is a failure view.
 *
 * @note    RTAB(0) is REM (Green Book 2.11).
 * @see     sno_rem()
 */
sno_view_t sno_rtab(sno_view_t* u, int n);

/**
 * 2.11 SNOBOL REM
 * Match the rest of the subject (u), from the cursor to the end.
 *
 * @pre     u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *
 * Success: returns the matched view [old cursor, end); modifies the subject -
 *          cursor begin moves to the end, leaving the subject a null string.
 *  + cursor already at end: returns a null string at the cursor;
 *    the subject is unchanged.
 * Failure: returns sno_fail().
 *  + The subject is a failure view.
 *
 * @note    Never fails on a real subject: REM always matches, if only the null string.
 * @note    REM is RTAB(0) (Green Book 2.11); its precondition comes from sno_rtab().
 * @see     sno_rtab(), sno_len()
 */
static inline sno_view_t sno_rem(sno_view_t* u) { return sno_rtab(u, 0); }

/**
 * 2.11 SNOBOL TAB(integer)
 * Match up through the nth character of the view (u), counting from its begin.
 *
 * @pre     u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 * @pre     n >= 0, else error 14 NEGATIVE NUMBER IN ILLEGAL CONTEXT
 *
 * Success: returns the matched view [begin, begin + n); modifies the subject -
 *          begin moves to column n, leaving u as column n to end.
 *  + n == 0: returns a null string at begin; the subject is unchanged.
 * Failure: returns sno_fail(); leaves the subject unchanged.
 *  + n is greater than the size of u.
 *  + The subject is a failure view.
 *
 * @warning Columns count from u's begin, which is the cursor. On a fresh view
 *          (just read or bound) that is column n of the line; after other
 *          matches it is n characters further on, i.e. LEN(n). For true
 *          columns, TAB from a fresh copy:  u = line; sno_tab(&u, 29);
 * @note    Unlike Green Book TAB, it cannot fail for "cursor already past n":
 *          a view has no position left of its begin.
 * @see     sno_rtab(), sno_len()
 */
sno_view_t sno_tab(sno_view_t* u, int n);

/**
 * 2.12 SNOBOL RPOS(integer)
 * Test that exactly n characters of the subject (u) remain after the cursor.
 *
 * @pre     u != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 * @pre     n >= 0, else error 14 NEGATIVE NUMBER IN ILLEGAL CONTEXT
 *
 * Success: returns a null string at the cursor; the subject is never modified.
 *  + exactly n characters remain.
 *  + n == 0: the cursor is at the end - the right anchor, "whole subject matched".
 * Failure: returns sno_fail(); leaves the subject unchanged.
 *  + more or fewer than n characters remain.
 *  + The subject is a failure view.
 *
 * @note    Never moves the cursor - it tests its position (Green Book 2.12).
 * @note    RTAB(n) moves the cursor to exactly where RPOS(n) succeeds.
 * @see     sno_rtab(), sno_at()
 */
sno_view_t sno_rpos(sno_view_t* u, int n);

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
