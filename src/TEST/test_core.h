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
 * Success advances begin by one and leaves end alone;
 * failure leaves the subject unchanged.
 * Preconditions: subject pointer != NULL; set not the null string
 * (Green Book error 4, NULL STRING IN ILLEGAL CONTEXT).
 */
static void test_any_notany() {
    static const char z[] = { 'A', '\0', 'B' };
    sno_view_t s, before, set;

    // ANY: match advances by one
    s = sno_bind("EXIT");
    before = s;
    assert(sno_any(&s, sno_bind("AEIOU")));
    assert(s.begin == before.begin + 1);
    assert(s.end == before.end);

    // ANY: only the first char counts
    s = sno_bind("XA");
    before = s;
    assert(!sno_any(&s, sno_bind("A")));
    assert(view_same(s, before));

    // ANY: order and duplicates irrelevant
    s = sno_bind("E");
    assert(sno_any(&s, sno_bind("UOIEA")));
    s = sno_bind("E");
    assert(sno_any(&s, sno_bind("AAEEAA")));

    // ANY: walks the subject, then fails at end
    s = sno_bind("AEI");
    set = sno_bind("AEIOU");
    assert(sno_any(&s, set));
    assert(sno_any(&s, set));
    assert(sno_any(&s, set));
    assert(sno_is_null(s));
    before = s;
    assert(!sno_any(&s, set));                  // cursor at end
    assert(view_same(s, before));

    // ANY: empty subject fails
    s = sno_null();
    before = s;
    assert(!sno_any(&s, sno_bind("A")));
    assert(view_same(s, before));

    // ANY: failure views propagate
    s = sno_bind("A");
    before = s;
    assert(!sno_any(&s, sno_fail()));
    assert(view_same(s, before));
    s = sno_fail();
    assert(!sno_any(&s, sno_bind("A")));
    assert(sno_failed(s));

    // ANY: NUL and high-bit chars are ordinary members
    set.begin = z;
    set.end = z + 3;                            // set { 'A', '\0', 'B' }
    s.begin = z + 1;
    s.end = z + 3;                              // subject "\0B"
    assert(sno_any(&s, set));
    assert(s.begin == z + 2);
    s = sno_bind("\xFF");
    assert(sno_any(&s, sno_bind("\xA0\xFF")));

    // NOTANY: match advances by one
    s = sno_bind("XA");
    before = s;
    assert(sno_notany(&s, sno_bind("AEIOU")));
    assert(s.begin == before.begin + 1);
    assert(s.end == before.end);

    // NOTANY: first char in set fails
    s = sno_bind("AX");
    before = s;
    assert(!sno_notany(&s, sno_bind("AEIOU")));
    assert(view_same(s, before));

    // NOTANY: fails at end - no char to match
    s = sno_null();
    before = s;
    assert(!sno_notany(&s, sno_bind("A")));
    assert(view_same(s, before));

    // NOTANY: Green Book example - STRUCTURE == CERSTU
    s = sno_bind("A");
    assert(sno_notany(&s, sno_bind("STRUCTURE")));
    s = sno_bind("A");
    assert(sno_notany(&s, sno_bind("CERSTU")));
    s = sno_bind("T");
    assert(!sno_notany(&s, sno_bind("STRUCTURE")));
    s = sno_bind("T");
    assert(!sno_notany(&s, sno_bind("CERSTU")));

    // NOTANY: failure views propagate
    s = sno_bind("X");
    before = s;
    assert(!sno_notany(&s, sno_fail()));
    assert(view_same(s, before));
    s = sno_fail();
    assert(!sno_notany(&s, sno_bind("A")));
    assert(sno_failed(s));

    // NOTANY: NUL is an ordinary member
    set.begin = z + 1;
    set.end = z + 2;                            // set { '\0' }
    s.begin = z + 1;
    s.end = z + 3;                              // subject "\0B"
    assert(!sno_notany(&s, set));
    s.begin = z + 2;                            // subject "B"
    assert(sno_notany(&s, set));
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
    printf("Pass\n");
}

#endif
