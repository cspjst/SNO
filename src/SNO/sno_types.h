#ifndef SNO_TYPES_H
#define SNO_TYPES_H

/**
 * A read-only position in text — no writing through a cursor.
 */
typedef const char* sno_cursor_t;

/**
 * @brief A read-only half-open interval [begin, end) view of part or all of a C string.
 *
 * The characters are read-only but the bounds are not.
 * A view can be shrunk or advanced by moving begin and end.
 * A view never includes the terminating '\0' but may contain embedded NUL bytes.
 * An empty view is a valid zero-length view, distinct from failure.
 *
 * Behaviours:
 * + Success, empty    begin != NULL, begin == end    sno_null(), sno_bind("")
 * + Success, nonempty begin != NULL, begin <  end    sno_bind("abc")
 * + Failure           begin == NULL, end == NULL     sno_fail(), sno_bind(NULL)
 */
typedef struct {
    sno_cursor_t begin;  // first char and the cursor when matching
    sno_cursor_t end;    // one past the last char (exclusive)
} sno_view_t;

#endif
