#ifndef TEST_CORE_H
#define TEST_CORE_H

#include "../SNO/sno_core.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Tests for sno_core.h (v0.2).
 *
 * Two kinds of bad input, as the Green Book (10.2.2):
 *   failure        the text doesn't match: functions return false or
 *                  sno_fail(), and failure views propagate
 *   precondition   a programmer error (Green Book "must"): assert
 *
 * Preconditions abort, so they are listed in each test's comment
 * but not exercised.
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

/*
 * sno_unbind(v, buf, size): copies at most size - 1 chars and always
 * terminates. Returns NULL, writing nothing, for size == 0 or a
 * failure view. Each case checks the byte after the expected write.
 * Precondition: buf != NULL
 */
static void test_unbind() {
    char buf[16];
    sno_view_t v = sno_bind("HELLO");

    v.begin++;
    v.end--;                                    // "ELL"

    memset(buf, 'X', sizeof buf);               // plenty of room
    assert(sno_unbind(v, buf, sizeof buf) == buf);
    assert(strcmp(buf, "ELL") == 0);
    assert(buf[4] == 'X');

    memset(buf, 'X', sizeof buf);               // exact fit: 3 chars + NUL
    assert(sno_unbind(v, buf, 4) == buf);
    assert(strcmp(buf, "ELL") == 0);
    assert(buf[4] == 'X');

    memset(buf, 'X', sizeof buf);               // one short: truncates
    assert(sno_unbind(v, buf, 3) == buf);
    assert(strcmp(buf, "EL") == 0);
    assert(buf[3] == 'X');

    memset(buf, 'X', sizeof buf);               // room for NUL only
    assert(sno_unbind(v, buf, 1) == buf);
    assert(buf[0] == '\0' && buf[1] == 'X');

    memset(buf, 'X', sizeof buf);               // size 0: NULL, untouched
    assert(sno_unbind(v, buf, 0) == NULL);
    assert(buf[0] == 'X');

    memset(buf, 'X', sizeof buf);               // null string gives ""
    assert(sno_unbind(sno_null(), buf, sizeof buf) == buf);
    assert(buf[0] == '\0' && buf[1] == 'X');

    memset(buf, 'X', sizeof buf);               // failure: NULL, untouched
    assert(sno_unbind(sno_fail(), buf, sizeof buf) == NULL);
    assert(buf[0] == 'X');
}

/*
 * 1.3.4 OUTPUT: sno_output(f, v) writes the view, no newline added.
 * Precondition: f != NULL
 */
static void test_output() {
    char buf[16];
    FILE* f = tmpfile();

    assert(f != NULL);

    assert(sno_output(f, sno_bind("HELLO")) == 5);
    assert(sno_output(f, sno_null()) == 0);
    assert(sno_output(f, sno_fail()) == 0);
    assert(!ferror(f));

    rewind(f);
    assert(fread(buf, 1, sizeof buf, f) == 5);
    assert(memcmp(buf, "HELLO", 5) == 0);

    fclose(f);
}

/*
 * 1.3.4 INPUT: sno_input(f, buf, size) reads exactly one line.
 * The view excludes the '\0', but buf holds it: *v.end == '\0'.
 * Chars beyond size - 1 are discarded. Failure at EOF only.
 * Preconditions: f != NULL, buf != NULL, size >= 2
 */
static void test_input() {
    char area[12];                              // 8 used, 4 canary
    sno_view_t v;
    FILE* f = tmpfile();

    assert(f != NULL);

    fputs("ONE\n\nSEVENCH\nTOOLONGLINE\nLAST", f);
    rewind(f);
    memset(area, 'X', sizeof area);

    v = sno_input(f, area, 8);
    assert(view_eq(v, "ONE"));                  // newline not stored
    assert(v.begin == area);
    assert(*v.end == '\0');                     // terminated, outside view
    assert(sno_ident(v, sno_bind("ONE")));      // agrees with bind

    v = sno_input(f, area, 8);                  // empty line: null string
    assert(v.begin == area && sno_is_null(v));
    assert(*v.end == '\0');

    v = sno_input(f, area, 8);                  // exactly fills buffer
    assert(view_eq(v, "SEVENCH"));
    assert(*v.end == '\0');

    v = sno_input(f, area, 8);                  // no phantom empty line;
    assert(view_eq(v, "TOOLONG"));              // long line truncated,
    assert(*v.end == '\0');                     // rest discarded

    v = sno_input(f, area, 8);                  // last line, no newline
    assert(view_eq(v, "LAST"));
    assert(*v.end == '\0');

    v = sno_input(f, area, 8);                  // EOF is failure
    assert(sno_failed(v));
    assert(feof(f) && !ferror(f));

    v = sno_input(f, area, 8);                  // and stays failure
    assert(sno_failed(v));

    assert(memcmp(area + 8, "XXXX", 4) == 0);   // no overrun

    fclose(f);                                  // trailing '\n' then EOF:
    f = tmpfile();                              // no extra empty line
    assert(f != NULL);
    fputs("A\n", f);
    rewind(f);
    assert(view_eq(sno_input(f, area, 8), "A"));
    assert(sno_failed(sno_input(f, area, 8)));

    fclose(f);                                  // blank last line is real
    f = tmpfile();
    assert(f != NULL);
    fputs("A\n\n", f);
    rewind(f);
    assert(view_eq(sno_input(f, area, 8), "A"));
    v = sno_input(f, area, 8);
    assert(v.begin == area && sno_is_null(v));
    assert(sno_failed(sno_input(f, area, 8)));

    fclose(f);                                  // empty file
    f = tmpfile();
    assert(f != NULL);
    assert(sno_failed(sno_input(f, area, 8)));

    fclose(f);
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

// Precondition violations: each must halt with an assert.
// Uncomment ONE line at a time; a pass is an abort, not "Pass".
static void test_sno_halt() {
    char buf[8];
    sno_view_t s = sno_bind("X");

    //sno_char_in_cstr('a', NULL);                        // charset NULL
    //sno_unbind(s, NULL, sizeof buf);                    // buf NULL
    //sno_unbind(sno_fail(), NULL, sizeof buf);           // buf NULL beats failure
    //sno_output(NULL, s);                                // f NULL (error 12)
    //sno_input(NULL, buf, sizeof buf);                   // f NULL (error 12)
    //sno_input(tmpfile(), NULL, sizeof buf);             // buf NULL
    //sno_input(tmpfile(), buf, 1);                       // size < 2
    //sno_input(tmpfile(), buf, 0);                       // size < 2
    //sno_input(fopen("halt.tmp", "w"), buf, sizeof buf); // read error (error 11)
    //sno_any(NULL, sno_bind("A"));                       // subject pointer NULL
    //sno_any(&s, sno_null());                            // null set (error 4)
    //sno_notany(NULL, sno_bind("A"));                    // subject pointer NULL
    //sno_notany(&s, sno_null());                         // null set (error 4)
}

static void test_core() {
    test_fail_null();
    test_char_in();
    test_bind();
    test_size();
    test_unbind();
    test_output();
    test_input();
    test_trim();
    test_any_notany();
    test_ident_differ();
    test_len();
    test_rtab();
    test_rem();
    //test_sno_halt();
    printf("Pass\n");
}

#endif
