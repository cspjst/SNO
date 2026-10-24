/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.1
 *
 * @brief SNOBOL4 Pattern Matching Library for C - Extra Functions
 *
 * The functions in sno_core.h embody SNOBOL semantics using views - sno_core.h never writes a byte.
 * Everything in sno_core.h is a view of text someone else owns.
 * Conversely, the functions here cross back into C.
 * They read streams or write NUL-terminated C strings into a caller's buf/size.
 *
 * Shared conventions:
 *  + buf == NULL is error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION.
 *  + A result that won't fit in buf is error 15 STRING OVERFLOW.
 *
 * Functions:
 *  + 1.1   C UNBIND          - copy a view into buf as a C string
 *  + 1.3.4 SNOBOL INPUT      - read one line into buf as a view
 *  + 1.3.4 SNOBOL OUTPUT     - write a view to a stream
 *  + 1.5   C PROXY           - subject pattern = object, into buf
 *  + 3.4.2 SNOBOL REPLACE    - character translation, into buf
 *  +       SNOBOL DUPL       - n copies of a string, into buf
 *  +       concatenation     - join views into buf
 *
 * @see sno_core.h, sno_error.h
 */
#ifndef SNO_EXTRA_H
#define SNO_EXTRA_H

#include "sno_types.h"

#include <stdio.h>

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
 * @pre
 *  + f != NULL, else error 12 ILLEGAL I/O UNIT
 *  + buf != NULL and size >= 2, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *  + no read error, else error 11 READING ERROR
 *
 * Success: returns a view of the line at buf.
 *  + Empty line returns a null string (size 0).
 *  + A final line without '\n' is returned as a normal line.
 *  + A line longer than size - 1 is truncated and the remainder is discarded.
 * Failure: returns sno_fail().
 *  + EOF before any character is read.
 *
 * @note
 *  + One call reads exactly one line, so a read loop always terminates.
 *  + Possibly truncated if sno_size(v) == size - 1.
 * @see     sno_output(), sno_trim()
 */
sno_view_t sno_input(FILE* f, char* buf, size_t size);

/**
 * 1.5 C PROXY
 * Expand a template in place: each occurrence of a marker from[i] in the C string in buf is replaced by to[i].
 *
 * @pre
 *  + buf != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *  + buf holds a '\0' within size, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *  + from != NULL and to != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *  + the result fits in size, '\0' included, else error 15 STRING OVERFLOW
 *
 * Success: returns a view of the expanded buf; buf is terminated with '\0', which the view excludes.
 *  + No markers in buf: buf is unchanged.
 *  + buf "" gives a null string at buf.
 * Failure: returns sno_fail(); buf is unchanged.
 *  + from and to have different lengths - as REPLACE (Green Book 3.4.2).
 *  + from is empty, or a marker is "" - as REPLACE's null argument.
 *
 * @warning A to[i] must not point into buf.
 * @note
 *  + from and to are NULL-terminated lists of C strings.
 *  + Scans left to right; at each position the first marker in from that matches wins - SNOBOL alternation,
 *    so list "$10" before "$1".
 *  + Single pass: a marker inside a to[i] is copied, not expanded again.
 *  + Text matching no marker is copied as is - with markers "$0" ... "$9", "jmp $" is safe.
 *  + Error 15 only when there is a result to form: on failure nothing is formed (Green Book 10.2.3).
 *  + In the spirit of the replacement statement (Green Book 1.5): subject pattern = object.
 * @see     sno_replace()
 */
sno_view_t sno_proxy(char* buf, size_t size, const char** from, const char** to);

/**
 * 3.4.2 SNOBOL REPLACE(X,Y,Z)
 * Translate characters in place: each character of the C string in buf that appears in from is replaced by the corresponding character in to.
 *
 * @pre
 *  + buf != NULL, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *  + buf holds a '\0' within size, else error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
 *
 * Success: returns a view of buf [buf, its '\0'), measured before translation.
 *  + Characters not in from are unchanged; the size never changes.
 *  + buf "" gives a null string at buf.
 * Failure: returns sno_fail(); buf is unchanged.
 *  + from and to have different sizes (Green Book 3.4.2).
 *  + from or to is the null string (Green Book 3.4.2) - failure, not error 4 as for ANY.
 *  + from or to is a failure view.
 *
 * @warning A '\0' in to ends the C string in buf early; the view is still right.
 * @note
 *  + A character repeated in from: the rightmost correspondence holds - REPLACE('FEET','EE','AO') is FOOT.
 *  + A character repeated in to is fine - REPLACE(S, '.,;:', '    ') blanks punctuation.
 *  + The terminator is not translated, so '\0' in from never matches - as sno_char_in_cstr().
 *  + No error 15: translation never changes the size. To keep the original, copy it first (sno_unbind()).
 * @see     sno_proxy()
 */
sno_view_t sno_replace(char* buf, size_t size, sno_view_t from, sno_view_t to);

#endif
