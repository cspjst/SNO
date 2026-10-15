#ifndef SNO_TYPES_H
#define SNO_TYPES_H

/**
 * The cursor acts in SNOBOL 'anchored' mode
 */
typedef const char* sno_cursor_t;

/**
 * The immutable string view is [cursor, eof)
 */
typedef struct {
    sno_cursor_t begin;  // Current parsing position (cursor)
    sno_cursor_t end;    // End of valid input (exclusive bound)
} sno_view_t;

#endif
