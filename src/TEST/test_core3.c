#include "test_core3.h"
#include "test_view.h"                          // view_eq(), view_same()

#include "../SNO/sno_core.h"

#include <stdio.h>
#include <assert.h>

/*
 * Tests for sno_core.h (v0.4), Green Book 3.3.1:
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
 * Each test's comment lists its preconditions. IDENT and DIFFER have none: they
 * take no subject pointer and accept any view, so this chapter has no
 * error tests. The real halt and
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

void test_core3(void) {
    test_ident_differ();
    printf("Pass core 3\n");
}
