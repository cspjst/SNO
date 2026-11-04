#include "test_core3.h"
#include "test_view.h"                          // view_eq(), view_same()

#include "../SNO/sno_core.h"

#include <stdio.h>
#include <assert.h>

/*
 * Tests for sno_core.h (v0.4), Green Book 3.3.1 - 3.4.3:
 * Chapter 3 - Primitive Functions, Predicates, and Operations.
 * Independent of the other chapters: its own error tests and runner, test_core3().
 *
 * Two kinds of bad input, as the Green Book (10.2.2):
 *   failure        the text doesn't match: functions return false or
 *                  sno_fail(), and failure views propagate
 *   precondition   a programmer error: sno_error() raises the Green Book
 *                  error number; under the default &ERRLIMIT = 0 it halts
 *                  with that number as ERRORLEVEL
 *
 * Each test's comment lists its preconditions. IDENT, DIFFER, SIZE and TRIM have
 * none: they take no subject pointer and accept any view, so this chapter
 * has no error tests. The real halt and
 * ERRORLEVEL are checked by the DOS batch file.
 */

/*
 * 3.3.1 IDENT and DIFFER: identity by content, not by pointer.
 * DIFFER is the complement of IDENT, except that both return
 * false when either argument is a failure view.
 */
void test_ident_differ(void) {
    static const char a[] = "BCD";
    static const char b[] = "xBCDx";
    static const char z1[] = { 'A', '\0', 'B' };
    static const char z2[] = { 'A', '\0', 'C' };
    sno_view_t bcd = sno_bind(a);
    sno_view_t mid, v, w;

    mid.begin = b + 1;                          // "BCD" in another buffer
    mid.end = b + 4;

    assert(sno_ident(bcd, bcd));                // same view
    assert(sno_ident(bcd, mid));                // same content, other pointers
    assert(sno_ident(mid, bcd));                // symmetric
    assert(!sno_differ(bcd, mid));

    assert(!sno_ident(sno_bind("PUSH"), sno_bind("PUSHI")));   // prefix
    assert(!sno_ident(sno_bind("PUSHI"), sno_bind("PUSH")));
    assert(sno_differ(sno_bind("PUSH"), sno_bind("PUSHI")));

    assert(!sno_ident(sno_bind("ACD"), bcd));   // differs first char
    assert(!sno_ident(sno_bind("BCE"), bcd));   // differs last char
    assert(!sno_ident(sno_bind("bcd"), bcd));   // case sensitive
    assert(sno_differ(sno_bind("bcd"), bcd));

    // every null string is identical to every other
    assert(sno_ident(sno_null(), sno_bind("")));
    assert(sno_ident(sno_bind(""), sno_null()));
    v.begin = v.end = b + 2;                    // empty view mid-buffer
    assert(sno_ident(v, sno_null()));
    assert(!sno_differ(v, sno_null()));
    assert(!sno_ident(sno_null(), bcd));        // DIFFER(X): X non-null
    assert(sno_differ(bcd, sno_null()));

    // views may hold NUL: compared like any other byte
    v.begin = z1;
    v.end = z1 + 3;
    w.begin = z2;
    w.end = z2 + 3;
    assert(!sno_ident(v, w));
    assert(sno_differ(v, w));
    v.end = z1 + 2;                             // "A\0" vs "A\0"
    w.end = z2 + 2;
    assert(sno_ident(v, w));

    assert(sno_ident(sno_bind("\xA0\xFF"), sno_bind("\xA0\xFF")));  // high bit
    assert(!sno_ident(sno_bind("\xA0"), sno_bind("\xFF")));

    // failure: neither succeeds (statement would fail in SNOBOL)
    assert(!sno_ident(sno_fail(), bcd));
    assert(!sno_ident(bcd, sno_fail()));
    assert(!sno_ident(sno_fail(), sno_fail()));
    assert(!sno_differ(sno_fail(), bcd));
    assert(!sno_differ(bcd, sno_fail()));
    assert(!sno_differ(sno_fail(), sno_fail()));
    assert(!sno_ident(sno_fail(), sno_null())); // failure is not ""
    assert(!sno_differ(sno_fail(), sno_null()));
}

// 3.4.1 SIZE
void test_size(void) {
    sno_view_t v = sno_bind("HELLO");

    assert(sno_size(v) == 5);
    v.begin++;                                  // "ELLO"
    assert(sno_size(v) == 4);
    v.end--;                                    // "ELL"
    assert(sno_size(v) == 3);
    v.end = v.begin;
    assert(sno_size(v) == 0);
    assert(sno_size(sno_fail()) == 0);
}

// 3.4.3 TRIM: trailing POSIX "C" locale whitespace only
void test_trim(void) {
    static const char s[] = "ABC  ";
    sno_view_t v;

    // each of the six spaces on its own
    assert(view_eq(sno_trim(sno_bind("ABC ")),  "ABC"));
    assert(view_eq(sno_trim(sno_bind("ABC\f")), "ABC"));
    assert(view_eq(sno_trim(sno_bind("ABC\n")), "ABC"));
    assert(view_eq(sno_trim(sno_bind("ABC\r")), "ABC"));
    assert(view_eq(sno_trim(sno_bind("ABC\t")), "ABC"));
    assert(view_eq(sno_trim(sno_bind("ABC\v")), "ABC"));

    assert(view_eq(sno_trim(sno_bind("ABC \f\n\r\t\v")), "ABC"));  // mixed
    assert(view_eq(sno_trim(sno_bind("ABC\r\n")), "ABC"));    // DOS line end
    assert(view_eq(sno_trim(sno_bind("  ABC")), "  ABC"));    // leading kept
    assert(view_eq(sno_trim(sno_bind("A \t B")), "A \t B"));  // interior kept
    assert(view_eq(sno_trim(sno_bind(" \t\r\n")), ""));       // all space
    assert(sno_is_null(sno_trim(sno_null())));

    // not spaces: other control and high-bit chars stay
    assert(view_eq(sno_trim(sno_bind("ABC\x1B")), "ABC\x1B"));
    assert(view_eq(sno_trim(sno_bind("ABC\xA0")), "ABC\xA0"));
    assert(view_eq(sno_trim(sno_bind("ABC\xFF")), "ABC\xFF"));

    v = sno_trim(sno_bind(s));
    assert(v.begin == s && v.end == s + 3);     // shrinks, no copy

    assert(sno_failed(sno_trim(sno_fail())));   // failure propagates
}

void test_core3(void) {
    test_ident_differ();
    test_size();
    test_trim();
    printf("Pass core 3\n");
}
