#ifndef TEST_CORE_H
#define TEST_CORE_H

#include "../SNO/sno_core.h"
#include "../SNO/sno_extra.h"
#include "../SNO/sno_error.h"
#include "../SNO/sno_constants.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Tests for sno_core.h (v0.4) - SNOBOL semantics in views.
 * Tests for sno_extra.h (unbind, input, output) are in test_extra.h.
 *
 * Two kinds of bad input, as the Green Book (10.2.2):
 *   failure        the text doesn't match: functions return false or
 *                  sno_fail(), and failure views propagate
 *   precondition   a programmer error: sno_error() halts with the
 *                  Green Book error number as ERRORLEVEL
 *
 * Preconditions halt, so they are listed in each test's comment and
 * exercised one at a time in test_sno_halt().
 */

// View holds exactly the characters of s (and is not a failure view)
static bool view_eq(sno_view_t v, const char* s) {
    size_t n = strlen(s);
    return !sno_failed(v)
        && (size_t)sno_size(v) == n
        && memcmp(v.begin, s, n) == 0;
}

// Same begin and same end
static bool view_same(sno_view_t a, sno_view_t b) {
    return a.begin == b.begin && a.end == b.end;
}

// 1.3.1 NULL and 1.8 Flow Control: null string versus failure
static void test_fail_null() {
    static const char s[] = "X";
    sno_view_t v;

    v = sno_fail();
    assert(sno_failed(v));
    assert(v.begin == NULL && v.end == NULL);   // the encoding
    assert(!sno_is_null(v));                    // failure is not ""
    assert(sno_size(v) == 0);

    v = sno_null();
    assert(!sno_failed(v));
    assert(sno_is_null(v));
    assert(v.begin != NULL && v.begin == v.end);
    assert(sno_size(v) == 0);

    assert(sno_is_null(sno_bind("")));          // many null strings
    assert(!sno_is_null(sno_bind("X")));
    assert(!sno_failed(sno_bind("X")));

    v.begin = v.end = s + 1;                    // empty view mid-buffer
    assert(sno_is_null(v));

    v.begin = NULL;                             // malformed: unsafe to read,
    v.end = s;                                  // so counts as failed
    assert(sno_failed(v));
    assert(!sno_is_null(v));
}

// Set membership helpers
// Precondition: sno_char_in_cstr charset != NULL
static void test_char_in() {
    static const char bytes[] = { 'A', '\0', 'B' };
    sno_view_t set = sno_bind("abc");
    sno_view_t v;

    assert(sno_char_in_set('a', set));          // first
    assert(sno_char_in_set('c', set));          // last
    assert(!sno_char_in_set('d', set));
    assert(!sno_char_in_set('\0', set));        // terminator not in view
    assert(!sno_char_in_set('a', sno_null()));  // empty set
    assert(!sno_char_in_set('a', sno_fail()));

    v.begin = bytes;                            // views may hold NUL
    v.end = bytes + 3;
    assert(sno_char_in_set('\0', v));
    assert(sno_char_in_set('B', v));

    v = sno_bind("\xA0\xFF");                   // high-bit chars
    assert(sno_char_in_set('\xA0', v));
    assert(sno_char_in_set('\xFF', v));

    assert(sno_char_in_cstr('a', "abc"));
    assert(sno_char_in_cstr('c', "abc"));
    assert(!sno_char_in_cstr('d', "abc"));
    assert(!sno_char_in_cstr('\0', "abc"));     // the strchr trap
    assert(!sno_char_in_cstr('a', ""));
    assert(sno_char_in_cstr('\xFF', "\xA0\xFF"));
}

// 1.1 sno_bind
static void test_bind() {
    static const char s[] = "TEST";
    static const char e[] = "";
    static const char z[] = "AB\0CD";
    sno_view_t v;

    v = sno_bind(s);
    assert(v.begin == s);                       // a view, not a copy
    assert(v.end == s + 4);
    assert(sno_size(v) == 4);

    v = sno_bind(e);                            // null string
    assert(v.begin == e && v.end == e);
    assert(sno_is_null(v));

    v = sno_bind(z);                            // stops at first NUL
    assert(v.end == z + 2);
    assert(sno_size(v) == 2);

    v = sno_bind(NULL);                         // bridge from C's NULL
    assert(sno_failed(v));
    assert(v.begin == NULL && v.end == NULL);
}

// 1.10.1 SIZE
static void test_size() {
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

// 1.11 TRIM: trailing POSIX "C" locale whitespace only
static void test_trim() {
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

/*
 * 2.10 ANY and NOTANY: match exactly one character.
 * Success returns the one-character match [old cursor, new cursor),
 * advances begin by one and leaves end alone.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL; set not the null string
 * (Green Book error 4, NULL STRING IN ILLEGAL CONTEXT).
 */
static void test_any_notany() {
    static const char z[] = { 'A', '\0', 'B' };
    sno_view_t s, before, set, m;

    // ANY: match is the one char, cursor advances by one
    s = sno_bind("EXIT");
    before = s;
    m = sno_any(&s, sno_bind("AEIOU"));
    assert(view_eq(m, "E"));
    assert(m.begin == before.begin);            // a view into the subject
    assert(m.end == s.begin);                   // ends at the new cursor
    assert(s.begin == before.begin + 1);
    assert(s.end == before.end);

    // ANY: only the first char counts
    s = sno_bind("XA");
    before = s;
    assert(sno_failed(sno_any(&s, sno_bind("A"))));
    assert(view_same(s, before));

    // ANY: order and duplicates irrelevant
    s = sno_bind("E");
    assert(view_eq(sno_any(&s, sno_bind("UOIEA")), "E"));
    s = sno_bind("E");
    assert(view_eq(sno_any(&s, sno_bind("AAEEAA")), "E"));

    // ANY: walks the subject, then fails at end
    s = sno_bind("AEI");
    set = sno_bind("AEIOU");
    assert(view_eq(sno_any(&s, set), "A"));
    assert(view_eq(sno_any(&s, set), "E"));
    assert(view_eq(sno_any(&s, set), "I"));
    assert(sno_is_null(s));
    before = s;
    assert(sno_failed(sno_any(&s, set)));       // cursor at end
    assert(view_same(s, before));

    // ANY: empty subject fails
    s = sno_null();
    before = s;
    assert(sno_failed(sno_any(&s, sno_bind("A"))));
    assert(view_same(s, before));

    // ANY: failure views propagate
    s = sno_bind("A");
    before = s;
    assert(sno_failed(sno_any(&s, sno_fail())));
    assert(view_same(s, before));
    s = sno_fail();
    assert(sno_failed(sno_any(&s, sno_bind("A"))));
    assert(sno_failed(s));

    // ANY: NUL and high-bit chars are ordinary members
    set.begin = z;
    set.end = z + 3;                            // set { 'A', '\0', 'B' }
    s.begin = z + 1;
    s.end = z + 3;                              // subject "\0B"
    m = sno_any(&s, set);
    assert(m.begin == z + 1 && m.end == z + 2); // matched the NUL
    assert(s.begin == z + 2);
    s = sno_bind("\xFF");
    assert(view_eq(sno_any(&s, sno_bind("\xA0\xFF")), "\xFF"));

    // NOTANY: match is the one char, cursor advances by one
    s = sno_bind("XA");
    before = s;
    m = sno_notany(&s, sno_bind("AEIOU"));
    assert(view_eq(m, "X"));
    assert(m.begin == before.begin && m.end == s.begin);
    assert(s.begin == before.begin + 1);
    assert(s.end == before.end);

    // NOTANY: first char in set fails
    s = sno_bind("AX");
    before = s;
    assert(sno_failed(sno_notany(&s, sno_bind("AEIOU"))));
    assert(view_same(s, before));

    // NOTANY: fails at end - no char to match
    s = sno_null();
    before = s;
    assert(sno_failed(sno_notany(&s, sno_bind("A"))));
    assert(view_same(s, before));

    // NOTANY: Green Book example - STRUCTURE == CERSTU
    s = sno_bind("A");
    assert(view_eq(sno_notany(&s, sno_bind("STRUCTURE")), "A"));
    s = sno_bind("A");
    assert(view_eq(sno_notany(&s, sno_bind("CERSTU")), "A"));
    s = sno_bind("T");
    assert(sno_failed(sno_notany(&s, sno_bind("STRUCTURE"))));
    s = sno_bind("T");
    assert(sno_failed(sno_notany(&s, sno_bind("CERSTU"))));

    // NOTANY: failure views propagate
    s = sno_bind("X");
    before = s;
    assert(sno_failed(sno_notany(&s, sno_fail())));
    assert(view_same(s, before));
    s = sno_fail();
    assert(sno_failed(sno_notany(&s, sno_bind("A"))));
    assert(sno_failed(s));

    // NOTANY: NUL is an ordinary member
    set.begin = z + 1;
    set.end = z + 2;                            // set { '\0' }
    s.begin = z + 1;
    s.end = z + 3;                              // subject "\0B"
    assert(sno_failed(sno_notany(&s, set)));
    s.begin = z + 2;                            // subject "B"
    assert(view_eq(sno_notany(&s, set), "B"));
}

/*
 * 3.3.1 IDENT and DIFFER: identity by content, not by pointer.
 * DIFFER is the complement of IDENT, except that both return
 * false when either argument is a failure view.
 */
static void test_ident_differ() {
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

/*
 * 2.8 LEN: match exactly n characters, whatever they are.
 * Success returns the matched view [old cursor, old cursor + n)
 * and advances begin by n; end is never moved.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL; n >= 0
 * (Green Book error 14, NEGATIVE NUMBER IN ILLEGAL CONTEXT).
 */
static void test_len() {
    static const char z[] = { 'A', '\0', '\xFF', 'B' };
    sno_view_t s, before, m;

    // match is a view into the subject, cursor advances by n
    s = sno_bind("SNOBOL4");
    before = s;
    m = sno_len(&s, 3);
    assert(view_eq(m, "SNO"));
    assert(m.begin == before.begin);            // a view, not a copy
    assert(m.end == s.begin);                   // ends at the new cursor
    assert(s.begin == before.begin + 3);
    assert(s.end == before.end);
    assert(view_eq(s, "BOL4"));

    // LEN(0): null string at the cursor, subject unchanged
    s = sno_bind("SNOBOL4");
    before = s;
    m = sno_len(&s, 0);
    assert(sno_is_null(m));                     // success, not failure
    assert(m.begin == before.begin);            // at the cursor
    assert(view_same(s, before));

    // exactly the remaining length: matches all, subject now null
    s = sno_bind("SNOBOL4");
    assert(view_eq(sno_len(&s, 7), "SNOBOL4"));
    assert(sno_is_null(s));

    // one more than remains: fails, subject unchanged
    s = sno_bind("SNOBOL4");
    before = s;
    assert(sno_failed(sno_len(&s, 8)));
    assert(view_same(s, before));

    // empty subject: LEN(0) succeeds, LEN(1) fails
    s = sno_null();
    before = s;
    assert(sno_is_null(sno_len(&s, 0)));
    assert(view_same(s, before));
    assert(sno_failed(sno_len(&s, 1)));
    assert(view_same(s, before));

    // Green Book 2.8 example: fixed-width fields from a card
    s = sno_bind("1290 SEP. 27 CHINA, CHIHLI");
    assert(view_eq(sno_len(&s, 4), "1290"));   // YR
    assert(view_eq(sno_len(&s, 1), " "));
    assert(view_eq(sno_len(&s, 4), "SEP."));   // MO
    assert(view_eq(sno_len(&s, 1), " "));
    assert(view_eq(sno_len(&s, 2), "27"));     // DAY
    assert(view_eq(s, " CHINA, CHIHLI"));

    // a failed LEN leaves the cursor usable for the next match
    s = sno_bind("AB");
    assert(sno_failed(sno_len(&s, 3)));
    assert(view_eq(sno_len(&s, 2), "AB"));

    // content-agnostic: NUL and high-bit chars count like any other
    s.begin = z;
    s.end = z + 4;
    m = sno_len(&s, 3);
    assert(m.begin == z && m.end == z + 3);     // 'A', '\0', '\xFF'
    assert(s.begin == z + 3);

    // failure subject propagates
    s = sno_fail();
    assert(sno_failed(sno_len(&s, 0)));         // even LEN(0)
    assert(sno_failed(sno_len(&s, 1)));
    assert(sno_failed(s));
}

/*
 * 2.11 RTAB: match from the cursor up to, but not including, the nth
 * character from the end. RTAB(0) is REM.
 * Success returns the matched view [old cursor, end - n) and moves
 * begin to end - n; end is never moved.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL (error 10); n >= 0
 * (error 14, NEGATIVE NUMBER IN ILLEGAL CONTEXT).
 */
static void test_rtab() {
    static const char z[] = { 'A', '\0', '\xFF', 'B' };
    sno_view_t s, before, m;

    // Green Book: 'SNOBOL4' LEN(2) RTAB(1) - RTAB matches OBOL
    s = sno_bind("SNOBOL4");
    assert(view_eq(sno_len(&s, 2), "SN"));
    before = s;
    m = sno_rtab(&s, 1);
    assert(view_eq(m, "OBOL"));
    assert(m.begin == before.begin);            // a view, not a copy
    assert(m.end == s.begin);                   // ends at the new cursor
    assert(s.end == before.end);                // end never moves
    assert(view_eq(s, "4"));

    // RTAB(0): the rest of the subject (REM), subject now null
    s = sno_bind("SNOBOL4");
    assert(view_eq(sno_rtab(&s, 0), "SNOBOL4"));
    assert(sno_is_null(s));

    // n == characters remaining: null string at the cursor, unchanged
    s = sno_bind("SNOBOL4");
    before = s;
    m = sno_rtab(&s, 7);
    assert(sno_is_null(m));                     // success, not failure
    assert(m.begin == before.begin);
    assert(view_same(s, before));

    // n > characters remaining: would move the cursor left - failure
    s = sno_bind("SNOBOL4");
    before = s;
    assert(sno_failed(sno_rtab(&s, 8)));
    assert(view_same(s, before));

    // measured from the end, not the start: cursor already past it fails
    s = sno_bind("SNOBOL4");
    assert(view_eq(sno_len(&s, 5), "SNOBO"));  // 2 left
    before = s;
    assert(sno_failed(sno_rtab(&s, 3)));
    assert(view_same(s, before));
    assert(view_eq(sno_rtab(&s, 2), ""));       // exactly at it: null match
    assert(view_eq(sno_rtab(&s, 1), "L"));

    // Green Book: RTAB(8) REM . L8 - split off the last eight characters
    s = sno_bind("DOMINICK V. DANIELS");
    assert(view_eq(sno_rtab(&s, 8), "DOMINICK V."));
    assert(view_eq(sno_rtab(&s, 0), " DANIELS"));   // L8
    assert(sno_is_null(s));

    // empty subject: RTAB(0) succeeds, RTAB(1) fails
    s = sno_null();
    before = s;
    assert(sno_is_null(sno_rtab(&s, 0)));
    assert(view_same(s, before));
    assert(sno_failed(sno_rtab(&s, 1)));
    assert(view_same(s, before));

    // a failed RTAB leaves the cursor usable for the next match
    s = sno_bind("AB");
    assert(sno_failed(sno_rtab(&s, 3)));
    assert(view_eq(sno_rtab(&s, 0), "AB"));

    // content-agnostic: NUL and high-bit chars count like any other
    s.begin = z;
    s.end = z + 4;
    m = sno_rtab(&s, 1);
    assert(m.begin == z && m.end == z + 3);     // 'A', '\0', '\xFF'
    assert(s.begin == z + 3);

    // failure subject propagates
    s = sno_fail();
    assert(sno_failed(sno_rtab(&s, 0)));        // even RTAB(0)
    assert(sno_failed(sno_rtab(&s, 1)));
    assert(sno_failed(s));
}

/*
 * 2.11 REM: match the rest of the subject. REM is RTAB(0).
 * Success returns the matched view [old cursor, end) and leaves the
 * subject null at its end; only a failure-view subject fails.
 * Precondition: subject pointer != NULL (error 10).
 */
static void test_rem() {
    static const char z[] = { 'A', '\0', '\xFF' };
    sno_view_t s, before, m;

    // whole subject: matched, subject now null at its end
    s = sno_bind("SNOBOL4");
    before = s;
    m = sno_rem(&s);
    assert(view_eq(m, "SNOBOL4"));
    assert(m.begin == before.begin);            // a view, not a copy
    assert(m.end == before.end);
    assert(sno_is_null(s));
    assert(s.begin == before.end);              // cursor at the end
    assert(s.end == before.end);                // end never moves

    // after other matches: the remainder only
    s = sno_bind("SNOBOL4");
    assert(view_eq(sno_len(&s, 2), "SN"));
    assert(view_eq(sno_rem(&s), "OBOL4"));
    assert(sno_is_null(s));

    // at the end: succeeds with a null string, never fails
    before = s;
    m = sno_rem(&s);
    assert(sno_is_null(m));                     // success, not failure
    assert(m.begin == before.begin);
    assert(view_same(s, before));
    assert(sno_is_null(sno_rem(&s)));           // and again

    // empty subject: null string
    s = sno_null();
    before = s;
    assert(sno_is_null(sno_rem(&s)));
    assert(view_same(s, before));

    // Green Book 2.11: RTAB(8) REM . L8
    s = sno_bind("DOMINICK V. DANIELS");
    assert(view_eq(sno_rtab(&s, 8), "DOMINICK V."));
    assert(view_eq(sno_rem(&s), " DANIELS"));   // L8

    // same as RTAB(0)
    s = sno_bind("PUSHI");
    m = sno_rem(&s);
    s = sno_bind("PUSHI");
    assert(sno_ident(m, sno_rtab(&s, 0)));

    // content-agnostic: NUL and high-bit chars included
    s.begin = z;
    s.end = z + 3;
    m = sno_rem(&s);
    assert(m.begin == z && m.end == z + 3);
    assert(sno_is_null(s));

    // failure subject propagates
    s = sno_fail();
    assert(sno_failed(sno_rem(&s)));
    assert(sno_failed(s));
}

/*
 * 2.11 TAB: match up through the nth character of the view, counting
 * from its begin. Success returns [begin, begin + n) and leaves the
 * subject as "column n to end"; end is never moved.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL (error 10); n >= 0 (error 14).
 * Columns are of the view passed in: TAB from a fresh copy of the line.
 */
static void test_tab() {
    // Green Book 2.11 card: cols 1-3 district, 4-29 name, 30-35 party, 36- PO
    static const char card[] = "  1WILLIAM T. CAHILL         REP   COLLINGSWOOD";
    static const char z[] = { 'A', '\0', '\xFF', 'B' };
    sno_view_t line = sno_bind(card);
    sno_view_t s, before, m;

    // fresh view: TAB(n) matches columns 1..n, leaves column n to end
    s = line;
    m = sno_tab(&s, 3);
    assert(view_eq(m, "  1"));
    assert(m.begin == line.begin);              // a view, not a copy
    assert(m.end == s.begin);                   // ends at the new cursor
    assert(s.begin == line.begin + 3);
    assert(s.end == line.end);                  // end never moves

    // the Green Book fields, each TABbed from a fresh copy of the line
    s = line;
    sno_tab(&s, 3);
    assert(view_eq(sno_len(&s, 26), "WILLIAM T. CAHILL         "));   // NAME
    s = line;
    sno_tab(&s, 35);
    assert(view_eq(sno_rem(&s), "COLLINGSWOOD"));                     // PO

    // TAB(0): null string at begin, subject unchanged
    s = line;
    m = sno_tab(&s, 0);
    assert(sno_is_null(m));                     // success, not failure
    assert(m.begin == line.begin);
    assert(view_same(s, line));

    // TAB(size): whole view, subject now null at its end
    s = sno_bind("SNOBOL4");
    assert(view_eq(sno_tab(&s, 7), "SNOBOL4"));
    assert(sno_is_null(s));

    // n > size: failure, subject unchanged
    s = sno_bind("SNOBOL4");
    before = s;
    assert(sno_failed(sno_tab(&s, 8)));
    assert(view_same(s, before));

    // @warning: chained after other matches, TAB counts from the new
    // begin - TAB(3) then TAB(29) lands on column 32, i.e. LEN(29)
    s = line;
    sno_tab(&s, 3);
    m = sno_tab(&s, 29);
    assert(m.begin == line.begin + 3);
    assert(s.begin == line.begin + 32);
    assert(view_eq(s, "   COLLINGSWOOD"));

    // so on any view, TAB(n) and LEN(n) agree
    s = sno_bind("SNOBOL4");
    sno_len(&s, 2);
    before = s;
    m = sno_tab(&s, 4);
    s = before;
    assert(sno_ident(m, sno_len(&s, 4)));

    // empty subject: TAB(0) succeeds, TAB(1) fails
    s = sno_null();
    before = s;
    assert(sno_is_null(sno_tab(&s, 0)));
    assert(view_same(s, before));
    assert(sno_failed(sno_tab(&s, 1)));
    assert(view_same(s, before));

    // content-agnostic: NUL and high-bit chars count like any other
    s.begin = z;
    s.end = z + 4;
    m = sno_tab(&s, 3);
    assert(m.begin == z && m.end == z + 3);     // 'A', '\0', '\xFF'
    assert(s.begin == z + 3);

    // failure subject propagates
    s = sno_fail();
    assert(sno_failed(sno_tab(&s, 0)));         // even TAB(0)
    assert(sno_failed(sno_tab(&s, 1)));
    assert(sno_failed(s));
}

/*
 * 2.12 RPOS: test that exactly n characters remain after the cursor.
 * Success returns a null string at the cursor; failure returns sno_fail().
 * Either way the subject is never modified.
 * Preconditions: subject pointer != NULL (error 10); n >= 0 (error 14).
 */
static void test_rpos() {
    static const char z[] = { 'A', '\0', '\xFF' };
    sno_view_t s, before, m;
    int n;

    // exactly n remain: null string at the cursor, subject unchanged
    s = sno_bind("SNOBOL4");
    before = s;
    m = sno_rpos(&s, 7);
    assert(sno_is_null(m));                     // true is the null string
    assert(m.begin == s.begin);                 // at the cursor
    assert(view_same(s, before));               // never moves

    // more or fewer than n: failure, subject unchanged
    assert(sno_failed(sno_rpos(&s, 6)));        // more remain
    assert(view_same(s, before));
    assert(sno_failed(sno_rpos(&s, 8)));        // fewer remain
    assert(view_same(s, before));

    // Green Book 2.12 shape: 'SNOBOL4' LEN(2) RPOS(5)
    s = sno_bind("SNOBOL4");
    assert(view_eq(sno_len(&s, 2), "SN"));
    before = s;
    assert(sno_is_null(sno_rpos(&s, 5)));
    assert(view_same(s, before));
    assert(sno_failed(sno_rpos(&s, 4)));

    // RPOS(0): the right anchor - succeeds only at the end
    s = sno_bind("SNOBOL4");
    assert(sno_failed(sno_rpos(&s, 0)));        // not at the end yet
    assert(view_eq(sno_len(&s, 7), "SNOBOL4"));
    assert(sno_is_null(sno_rpos(&s, 0)));       // whole subject matched

    // ENTIRE idiom: a chain that must consume everything
    s = sno_bind("AB");
    assert(view_eq(sno_len(&s, 1), "A"));
    assert(sno_failed(sno_rpos(&s, 0)));        // "B" left over
    assert(view_eq(sno_rem(&s), "B"));
    assert(sno_is_null(sno_rpos(&s, 0)));

    // after RTAB(n) succeeds, RPOS(n) always succeeds
    for (n = 0; n <= 7; n++) {
        s = sno_bind("SNOBOL4");
        assert(!sno_failed(sno_rtab(&s, n)));
        assert(sno_is_null(sno_rpos(&s, n)));
    }

    // asking twice changes nothing
    s = sno_bind("XY");
    before = s;
    assert(sno_is_null(sno_rpos(&s, 2)));
    assert(sno_is_null(sno_rpos(&s, 2)));
    assert(view_same(s, before));

    // empty subject: RPOS(0) succeeds, RPOS(1) fails
    s = sno_null();
    before = s;
    assert(sno_is_null(sno_rpos(&s, 0)));
    assert(sno_failed(sno_rpos(&s, 1)));
    assert(view_same(s, before));

    // content-agnostic: NUL and high-bit chars count like any other
    s.begin = z;
    s.end = z + 3;
    assert(sno_is_null(sno_rpos(&s, 3)));
    assert(sno_failed(sno_rpos(&s, 2)));

    // failure subject propagates
    s = sno_fail();
    assert(sno_failed(sno_rpos(&s, 0)));        // even RPOS(0)
    assert(sno_failed(s));
}

// 1.8 SUCCEEDED: the exact complement of sno_failed() - the null string succeeds
static void test_succeeded() {
    static const char s[] = "X";
    sno_view_t cases[6];
    sno_view_t v;
    int i;

    assert(!sno_succeeded(sno_fail()));
    assert(!sno_succeeded(sno_bind(NULL)));     // C's NULL is failure
    assert(sno_succeeded(sno_bind("X")));
    assert(sno_succeeded(sno_null()));          // the null string succeeds
    assert(sno_succeeded(sno_bind("")));

    v.begin = v.end = s + 1;                    // empty view mid-buffer
    assert(sno_succeeded(v));

    v.begin = NULL;                             // malformed {NULL, p}: failed,
    v.end = s;                                  // so does not succeed
    assert(!sno_succeeded(v));

    // the exact complement, case by case
    cases[0] = sno_fail();
    cases[1] = sno_null();
    cases[2] = sno_bind("");
    cases[3] = sno_bind("X");
    cases[4] = v;                               // malformed
    cases[5].begin = cases[5].end = s;          // empty at the start
    for (i = 0; i < 6; ++i)
        assert(sno_succeeded(cases[i]) != sno_failed(cases[i]));

    // in chains: success is not "matched something"
    v = sno_bind("SNOBOL4");
    assert(sno_succeeded(sno_len(&v, 3)));
    assert(sno_succeeded(sno_len(&v, 0)));      // null match succeeds
    assert(!sno_succeeded(sno_len(&v, 5)));     // only 4 left
    assert(sno_succeeded(sno_len(&v, 2)) && sno_succeeded(sno_rem(&v)));
    assert(sno_succeeded(sno_rem(&v)));         // REM at the end: null, succeeds
}

/*
 * 2.9 BREAK: match up to, but not including, the first break character.
 * Success returns [old cursor, break character) and moves the cursor to the
 * break character; at a break character it is a null string at the cursor.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL (error 10); set not the null
 * string (error 4, NULL STRING IN ILLEGAL CONTEXT).
 */
static void test_break() {
    static const char z[] = { 'A', '\0', 'B' };
    sno_view_t s, before, set, m;

    // the view's end is the end: no peeking past it into the buffer
    s = sno_bind("IT RUNS.");
    m = sno_break(&s, sno_bind(" "));           // m = "IT", a slice of the buffer
    before = m;
    assert(sno_failed(sno_break(&m, sno_bind("."))));   // the '.' is beyond m.end
    assert(view_same(m, before));

    // match is a view into the subject; the cursor stops AT the break char
    s = sno_bind("IT RUNS.");
    before = s;
    m = sno_break(&s, sno_bind(" "));
    assert(view_eq(m, "IT"));
    assert(m.begin == before.begin);            // a view, not a copy
    assert(m.end == s.begin);                   // ends at the new cursor
    assert(*s.begin == ' ');                    // the break char is not consumed
    assert(s.end == before.end);                // end never moves

    // Green Book 2.9 bead diagram: 'IT RUNS.' BREAK(' ') SPAN(' ') BREAK('.')
    // (LEN(1) stands in for SPAN(' ') until SPAN exists)
    assert(view_eq(sno_len(&s, 1), " "));
    assert(view_eq(sno_break(&s, sno_bind(".")), "RUNS"));
    assert(view_eq(s, "."));

    // at a break char: null string at the cursor, subject unchanged
    before = s;
    m = sno_break(&s, sno_bind("."));
    assert(sno_is_null(m));                     // success, not failure
    assert(m.begin == before.begin);            // at the cursor
    assert(view_same(s, before));

    // ...and again: BREAK alone never moves past it - consume it to go on
    assert(sno_is_null(sno_break(&s, sno_bind("."))));
    assert(view_same(s, before));

    // must find a break char: none before the end is failure, subject unchanged
    s = sno_bind("RUNS");
    before = s;
    assert(sno_failed(sno_break(&s, sno_bind("."))));
    assert(view_same(s, before));

    // empty subject: no break char to find
    s = sno_null();
    assert(sno_failed(sno_break(&s, sno_bind("."))));

    // the earliest break char in the subject wins; set order is irrelevant
    s = sno_bind("A,B;C");
    assert(view_eq(sno_break(&s, sno_bind(";,")), "A"));
    s = sno_bind("A,B;C");
    assert(view_eq(sno_break(&s, sno_bind(",;,;")), "A"));     // duplicates too

    // P86: mnemonic, operands, comment
    s = sno_bind("mov ax,bx ; copy");
    assert(view_eq(sno_break(&s, sno_bind(" ")), "mov"));
    sno_len(&s, 1);
    assert(view_eq(sno_break(&s, sno_bind(",")), "ax"));
    sno_len(&s, 1);
    assert(view_eq(sno_break(&s, sno_bind(" ;")), "bx"));
    s = sno_bind("push bp ; save");
    assert(view_eq(sno_break(&s, sno_bind(";")), "push bp "));

    // Green Book 2.9 table: the first field of a card
    s = sno_bind("ACTINIUM        AC       89");
    assert(view_eq(sno_break(&s, sno_bind(" ")), "ACTINIUM"));

    // NUL and high-bit chars are ordinary members
    set.begin = z + 1;
    set.end = z + 2;                            // set { '\0' }
    s.begin = z;
    s.end = z + 3;                              // subject "A\0B"
    m = sno_break(&s, set);
    assert(m.begin == z && m.end == z + 1);     // "A", stopped at the NUL
    assert(s.begin == z + 1);
    s = sno_bind("AB\xFF" "C");
    assert(view_eq(sno_break(&s, sno_bind("\xFF")), "AB"));

    // failure views propagate
    s = sno_bind("A.");
    before = s;
    assert(sno_failed(sno_break(&s, sno_fail())));
    assert(view_same(s, before));
    s = sno_fail();
    assert(sno_failed(sno_break(&s, sno_bind("."))));
    assert(sno_failed(s));
}

/*
 * 1.1 BIND LIST: bind a NULL-terminated list of C strings into a list of views
 * ending in a failure-view sentinel. Each view is sno_bind() of its string.
 * Preconditions: cstrs and views != NULL; size holds every view plus the
 * sentinel (error 10). On error nothing is written.
 */
static void test_bind_list() {
    static const char* ops[] = { "PUSHI", "PUSH", "POP", NULL };
    static const char* none[] = { NULL };
    static const char* blank[] = { "A", "", "B", NULL };
    sno_view_t canary = sno_bind("canary");
    sno_view_t v[8];
    int i;

    // each view is sno_bind of its string - a view of it, not a copy
    for (i = 0; i < 8; ++i)
        v[i] = canary;
    assert(sno_bind_list(ops, v, 8) == v);      // returns the list
    for (i = 0; i < 3; ++i) {
        assert(v[i].begin == ops[i]);
        assert(sno_ident(v[i], sno_bind(ops[i])));
    }
    assert(sno_failed(v[3]));                   // the sentinel
    assert(v[3].begin == NULL && v[3].end == NULL);

    // nothing written past the sentinel
    for (i = 4; i < 8; ++i)
        assert(view_same(v[i], canary));

    // walk it the way sno_any_lit will: up to the sentinel
    for (i = 0; S_(v[i]); ++i)
        ;
    assert(i == 3);

    // exact fit: n views + 1 sentinel
    assert(sno_bind_list(ops, v, 4) == v);
    assert(sno_failed(v[3]));

    // empty list: just the sentinel, size 1 is enough
    assert(sno_bind_list(none, v, 1) == v);
    assert(sno_failed(v[0]));

    // "" is the null string, not the end: the sentinel is a failure view
    assert(sno_bind_list(blank, v, 8) == v);
    assert(view_eq(v[0], "A"));
    assert(sno_is_null(v[1]) && !sno_failed(v[1]));
    assert(view_eq(v[2], "B"));
    assert(sno_failed(v[3]));
}

/*
 * 1.4 LIT: match a literal view at the cursor - SNOBOL's implicit string match.
 * Success returns the matched view [old cursor, old cursor + size) and advances
 * the cursor by its size. Failure returns sno_fail() and leaves the subject unchanged.
 * Precondition: subject pointer != NULL (error 10).
 */
static void test_lit() {
    static const char z[] = { 'A', '\0', 'B' };
    static const char nul[] = { '\0' };
    sno_view_t s, before, m, lit;

    // match is a view into the subject, cursor advances by its size
    s = sno_bind("mov ax,bx");
    before = s;
    m = sno_lit(&s, sno_bind("mov"));
    assert(view_eq(m, "mov"));
    assert(m.begin == before.begin);            // a view of the subject, not of lit
    assert(m.end == s.begin);                   // ends at the new cursor
    assert(s.end == before.end);                // end never moves
    assert(view_eq(s, " ax,bx"));

    // chains: each literal starts where the last one ended
    assert(view_eq(sno_lit(&s, sno_bind(" ")), " "));
    assert(view_eq(sno_lit(&s, sno_bind("ax")), "ax"));
    assert(view_eq(sno_lit(&s, sno_bind(",")), ","));
    assert(view_eq(sno_lit(&s, sno_bind("bx")), "bx"));
    assert(sno_is_null(s));

    // anchored: present later in the subject is not at the cursor - failure
    s = sno_bind("push bp");
    before = s;
    assert(sno_failed(sno_lit(&s, sno_bind("bp"))));
    assert(view_same(s, before));

    // a partial match fails and moves nothing
    s = sno_bind("ABD");
    before = s;
    assert(sno_failed(sno_lit(&s, sno_bind("ABC"))));
    assert(view_same(s, before));

    // longer than the subject, or case differs: failure
    s = sno_bind("AB");
    before = s;
    assert(sno_failed(sno_lit(&s, sno_bind("ABC"))));
    assert(sno_failed(sno_lit(&s, sno_bind("ab"))));
    assert(view_same(s, before));

    // the whole subject - even the subject itself as the literal
    s = sno_bind("PUSHI");
    assert(view_eq(sno_lit(&s, s), "PUSHI"));
    assert(sno_is_null(s));

    // the null string matches the null string at the cursor, even on an empty subject
    s = sno_bind("AB");
    before = s;
    m = sno_lit(&s, sno_null());
    assert(sno_is_null(m) && m.begin == before.begin);
    assert(view_same(s, before));
    s = sno_null();
    assert(sno_is_null(sno_lit(&s, sno_null())));
    assert(sno_failed(sno_lit(&s, sno_bind("A"))));     // but nothing else matches there

    // back-reference: match what was captured earlier
    s = sno_bind("ab:ab");
    m = sno_break(&s, sno_bind(":"));           // capture "ab"
    sno_len(&s, 1);                             // past the ':'
    assert(view_eq(sno_lit(&s, m), "ab"));
    assert(sno_is_null(s));
    s = sno_bind("ab:ac");
    m = sno_break(&s, sno_bind(":"));
    sno_len(&s, 1);
    before = s;
    assert(sno_failed(sno_lit(&s, m)));
    assert(view_same(s, before));

    // alternation is ||: a failed literal leaves the cursor for the next one
    s = sno_bind("BERATES");
    assert(S_(sno_lit(&s, sno_bind("GE"))) || S_(sno_lit(&s, sno_bind("FRE")))
        || S_(sno_lit(&s, sno_bind("BE"))));
    assert(view_eq(s, "RATES"));

    // the view's end is the end: no peeking past it into the buffer
    s = sno_bind("IT RUNS.");
    m = sno_break(&s, sno_bind(" "));           // m = "IT", a slice of the buffer
    before = m;
    assert(sno_failed(sno_lit(&m, sno_bind("IT "))));
    assert(view_same(m, before));

    // NUL is an ordinary char in a view: a literal can hold it and match it
    s.begin = z;
    s.end = z + 3;                              // subject "A\0B"
    lit.begin = z;
    lit.end = z + 2;                            // literal "A\0"
    m = sno_lit(&s, lit);
    assert(m.begin == z && m.end == z + 2);
    lit.begin = nul;
    lit.end = nul + 1;                          // literal "\0", a different buffer
    assert(sno_failed(sno_lit(&s, lit)));       // 'B' is not NUL
    s.begin = z + 1;                            // subject "\0B"
    assert(sno_size(sno_lit(&s, lit)) == 1);

    // high-bit chars are ordinary
    s = sno_bind("\xFF\xFE" "AB");
    assert(view_eq(sno_lit(&s, sno_bind("\xFF\xFE")), "\xFF\xFE"));

    // failure views: a failure literal is a value - it just fails
    s = sno_bind("AB");
    before = s;
    assert(sno_failed(sno_lit(&s, sno_fail())));
    assert(view_same(s, before));
    s = sno_fail();
    assert(sno_failed(sno_lit(&s, sno_bind("A"))));
    assert(sno_failed(sno_lit(&s, sno_null())));        // even the null literal
    assert(sno_failed(s));
}

/*
 * 1.4 FIND LIT: find which literal in a list matches at the cursor - dispatch.
 * Success returns the index of the first entry that matches and moves the cursor past it.
 * Failure returns -1 and leaves the subject unchanged. A list is C strings ending in NULL.
 * Preconditions: subject pointer and list != NULL (error 10).
 */
static void test_find_lit() {
    enum { PUSHI, PUSH, POP };                  // same order as MNEMONICS
    static const char* MNEMONICS[] = { "PUSHI", "PUSH", "POP", NULL };
    static const char* SHORT_FIRST[] = { "PUSH", "PUSHI", NULL };
    static const char* LINES[] = { "PUSHI 5", "PUSH x", "POP", "ADD" };
    sno_view_t s, before;
    int i, pushi = 0, push = 0, pop = 0, unknown = 0;

    // dispatch: the index of the entry that matched, the cursor past it
    s = sno_bind("PUSHI 5");
    assert(sno_find_lit(&s, MNEMONICS) == PUSHI && view_eq(s, " 5"));
    s = sno_bind("PUSH x");
    assert(sno_find_lit(&s, MNEMONICS) == PUSH && view_eq(s, " x"));
    s = sno_bind("POP");
    assert(sno_find_lit(&s, MNEMONICS) == POP && sno_is_null(s));

    // ...straight into a switch
    for (i = 0; i < 4; ++i) {
        s = sno_bind(LINES[i]);
        switch (sno_find_lit(&s, MNEMONICS)) {
        case PUSHI: ++pushi;   break;
        case PUSH:  ++push;    break;
        case POP:   ++pop;     break;
        default:    ++unknown; break;           // -1: no entry matched
        }
    }
    assert(pushi == 1 && push == 1 && pop == 1 && unknown == 1);

    // none matches - or only later in the subject (anchored): -1, subject unchanged
    s = sno_bind("ADD");
    before = s;
    assert(sno_find_lit(&s, MNEMONICS) == -1 && view_same(s, before));
    s = sno_bind("X PUSH");
    before = s;
    assert(sno_find_lit(&s, MNEMONICS) == -1 && view_same(s, before));

    // first in the list wins: PUSHI must come before PUSH
    s = sno_bind("PUSHI");
    assert(sno_find_lit(&s, SHORT_FIRST) == 0 && view_eq(s, "I"));

    // failure subject: -1, and it stays failed
    s = sno_fail();
    assert(sno_find_lit(&s, MNEMONICS) == -1 && sno_failed(s));
}
/*
 * Check order: argument errors first, then failure.
 * A bad argument (n < 0, null set, NULL pointer) is a bug in the program and
 * is raised on entry even if an earlier match failed - see test_sno_errors().
 * A failure view with *valid* arguments is normal flow: it just fails.
 */
static void test_fail_first() {
    sno_view_t s;

    // 1.4 LIT
    s = sno_fail();
    assert(sno_failed(sno_lit(&s, sno_bind("A"))));         // failure subject
    assert(sno_failed(sno_lit(&s, sno_null())));            // even the null literal
    s = sno_bind("A");
    assert(sno_failed(sno_lit(&s, sno_fail())));            // failure literal is a value: fails
    assert(view_eq(s, "A"));                                // subject untouched

    // 2.8 LEN
    s = sno_fail();
    assert(sno_failed(sno_len(&s, 1)));                     // failure subject
    assert(sno_failed(sno_len(&s, 0)));                     // even LEN(0)

    // 2.9 BREAK
    s = sno_fail();
    assert(sno_failed(sno_break(&s, sno_bind("A"))));       // failure subject
    assert(sno_failed(sno_break(&s, sno_fail())));          // failure subject and set
    s = sno_bind("A");
    assert(sno_failed(sno_break(&s, sno_fail())));          // failure set is a value: fails
    assert(view_eq(s, "A"));                                // subject untouched

    // 2.10 ANY / NOTANY
    s = sno_fail();
    assert(sno_failed(sno_any(&s, sno_bind("A"))));         // failure subject
    assert(sno_failed(sno_notany(&s, sno_bind("A"))));
    assert(sno_failed(sno_any(&s, sno_fail())));            // failure subject and set
    assert(sno_failed(sno_notany(&s, sno_fail())));
    s = sno_bind("A");
    assert(sno_failed(sno_any(&s, sno_fail())));            // failure set is a value: fails
    assert(sno_failed(sno_notany(&s, sno_fail())));
    assert(view_eq(s, "A"));                                // subject untouched

    // 2.11 TAB / RTAB / REM
    s = sno_fail();
    assert(sno_failed(sno_tab(&s, 0)));                     // failure subject
    assert(sno_failed(sno_rtab(&s, 0)));
    assert(sno_failed(sno_rem(&s)));

    // 2.12 RPOS
    s = sno_fail();
    assert(sno_failed(sno_rpos(&s, 0)));                    // failure subject
    assert(sno_failed(s));                                  // and it stays failed throughout
}

// Precondition violations under &ERRLIMIT (Green Book 10.2.2): with errors allowed, a
// conditionally fatal error is recorded for sno_err_type(), uses one of the limit, and the
// call fails instead of halting. Each line asserts its own error; the count at the end
// proves none was skipped. The real halt and ERRORLEVEL stay with the DOS batch file.
static void test_sno_errors() {
    static const char* ops[] = { "PUSHI", "PUSH", "POP", NULL };
    sno_view_t v[8];
    sno_view_t s = sno_bind("X");
    sno_view_t f = sno_fail();

    sno_err_limit(32);                          // exactly one per line below
    assert(sno_err_type() == SNO_ERR_NONE);     // the limit starts a clean record

    // 0.1 character sets   1
    assert(!sno_char_in_cstr('a', NULL)         && sno_err_type() == 10);  // charset NULL

    // 1.1 BIND LIST        5
    assert(sno_bind_list(NULL, v, 8) == NULL    && sno_err_type() == 10);  // cstrs NULL
    assert(sno_bind_list(ops, NULL, 8) == NULL  && sno_err_type() == 10);  // views NULL
    v[0] = sno_bind("X");
    assert(sno_bind_list(ops, v, 3) == NULL     && sno_err_type() == 10);  // no room for the sentinel
    assert(view_eq(v[0], "X"));                 // ...and nothing written
    assert(sno_bind_list(ops, v, 0) == NULL     && sno_err_type() == 10);  // size 0

    // 1.4 LIT              7
    assert(F_(sno_lit(NULL, sno_bind("A")))     && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_lit(NULL, sno_fail()))        && sno_err_type() == 10);  // subject pointer NULL beats failure

    // 1.4 FIND LIT         10
    assert(sno_find_lit(NULL, ops) == -1        && sno_err_type() == 10);  // subject pointer NULL
    assert(sno_find_lit(&s, NULL) == -1         && sno_err_type() == 10);  // list NULL
    assert(sno_find_lit(&f, NULL) == -1         && sno_err_type() == 10);  // list NULL beats failure

    // 2.8 LEN              13
    assert(F_(sno_len(NULL, 1))                 && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_len(&s, -1))                  && sno_err_type() == 14);  // n < 0
    assert(F_(sno_len(&f, -1))                  && sno_err_type() == 14);  // n < 0 beats failure

    // 2.9 BREAK            16
    assert(F_(sno_break(NULL, sno_bind(".")))   && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_break(&s, sno_null()))        && sno_err_type() == 4);   // null set
    assert(F_(sno_break(&f, sno_null()))        && sno_err_type() == 4);   // null set beats failure

    // 2.10 ANY / NOTANY    22
    assert(F_(sno_any(NULL, sno_bind("A")))     && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_any(&s, sno_null()))          && sno_err_type() == 4);   // null set
    assert(F_(sno_any(&f, sno_null()))          && sno_err_type() == 4);   // null set beats failure
    assert(F_(sno_notany(NULL, sno_bind("A")))  && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_notany(&s, sno_null()))       && sno_err_type() == 4);   // null set
    assert(F_(sno_notany(&f, sno_null()))       && sno_err_type() == 4);   // null set beats failure

    // 2.11 TAB / RTAB / REM    29
    assert(F_(sno_tab(NULL, 0))                 && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_tab(&s, -1))                  && sno_err_type() == 14);  // n < 0
    assert(F_(sno_tab(&f, -1))                  && sno_err_type() == 14);  // n < 0 beats failure
    assert(F_(sno_rtab(NULL, 0))                && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_rtab(&s, -1))                 && sno_err_type() == 14);  // n < 0
    assert(F_(sno_rtab(&f, -1))                 && sno_err_type() == 14);  // n < 0 beats failure
    assert(F_(sno_rem(NULL))                    && sno_err_type() == 10);  // subject pointer NULL, via RTAB

    // 2.12 RPOS            32
    assert(F_(sno_rpos(NULL, 0))                && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_rpos(&s, -1))                 && sno_err_type() == 14);  // n < 0
    assert(F_(sno_rpos(&f, -1))                 && sno_err_type() == 14);  // n < 0 beats failure

    assert(sno_err_count() == 0);               // all 32 raised, none skipped
    assert(view_eq(s, "X") && sno_failed(f));   // and no subject was moved
    sno_err_limit(0);                           // &ERRLIMIT = 0: errors halt again
}

static void test_core() {
    test_fail_null();
    test_char_in();
    test_bind();
    test_size();
    test_trim();
    test_any_notany();
    test_ident_differ();
    test_len();
    test_rtab();
    test_rem();
    test_tab();
    test_rpos();
    test_succeeded();
    test_break();
    test_bind_list();
    test_lit();
    test_find_lit();

    test_fail_first();
    test_sno_errors();
    printf("Pass core\n");
}

#endif
