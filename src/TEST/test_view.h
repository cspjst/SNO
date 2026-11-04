#ifndef TEST_VIEW_H
#define TEST_VIEW_H

#include "../SNO/sno_core.h"

#include <stdbool.h>
#include <string.h>

/*
 * View assertions shared by the test files.
 * static inline, so each .c file gets its own copy and nothing clashes at link time.
 */

// View holds exactly the characters of s (and is not a failure view)
static inline bool view_eq(sno_view_t v, const char* s) {
    size_t n = strlen(s);
    return !sno_failed(v)
        && (size_t)sno_size(v) == n
        && memcmp(v.begin, s, n) == 0;
}

// Same begin and same end
static inline bool view_same(sno_view_t a, sno_view_t b) {
    return a.begin == b.begin && a.end == b.end;
}

#endif
