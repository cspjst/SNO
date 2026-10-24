/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.1
 *
 * @brief SNOBOL4 Pattern Matching Library for C - Extra Functions
 *
 * Outside of the SNOBOL semantics in views sno_core.h never writes a byte.
 * Everything there is a view of text someone else owns.
 * Conversely, ehe functions here cross back into C.
 * They read streams or write NUL-terminated C strings into a caller's buf/size.
 *
 *
 * Shared conventions:
 *  + buf == NULL is error 10 ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION.
 *  + A result that won't fit in buf is error 15 STRING OVERFLOW.
 *
 * Functions:
 *  + 1.1   C UNBIND          - copy a view into buf as a C string (moving from sno_core.h)
 *  + 1.3.4 SNOBOL INPUT      - read one line into buf as a view
 *  + 1.3.4 SNOBOL OUTPUT     - write a view to a stream
 *  + 1.5   replacement       - subject pattern = object, into buf (planned; name TBC)
 *  + 3.4.2 SNOBOL REPLACE    - character translation, into buf (planned)
 *  +       SNOBOL DUPL       - n copies of a string, into buf (planned)
 *  +       concatenation     - join views into buf (planned)
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

#endif
