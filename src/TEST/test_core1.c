#include "test_core1.h"
#include "test_view.h"                          // view_eq(), view_same()

#include "../SNO/sno_core.h"
#include "../SNO/sno_error.h"

#include <stdio.h>
#include <assert.h>

/*
 * Tests for sno_core.h (v0.4), Green Book 0.1 - 1.11:
 * Chapter 0 - C Support and Chapter 1 - Introduction to the SNOBOL Programming Language.
 * Independent of the other chapters: its own error tests and runner, test_core1().
 *
 * Two kinds of bad input, as the Green Book (10.2.2):
 *   failure        the text doesn't match: functions return false or
 *                  sno_fail(), and failure views propagate
 *   precondition   a programmer error: sno_error() raises the Green Book
 *                  error number; under the default &ERRLIMIT = 0 it halts
 *                  with that number as ERRORLEVEL
 *
 * Each test's comment lists its preconditions. test_sno_errors1() raises
 * &ERRLIMIT so every violation is recorded and fails instead of halting,
 * then checks each error number. The real halt and
 * ERRORLEVEL are checked by the DOS batch file.
 */

// Set membership helpers
// Precondition: sno_char_in_cstr charset != NULL
void test_char_in(void) {
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
void test_bind(void) {
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

/*
 * 1.1 BIND LIST: bind a NULL-terminated list of C strings into a list of views
 * ending in a failure-view sentinel. Each view is sno_bind() of its string.
 * Preconditions: cstrs and views != NULL; size holds every view plus the
 * sentinel (error 10). On error nothing is written.
 */
void test_bind_list(void) {
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

// 1.3.1 NULL and 1.8 Flow Control: null string versus failure
void test_fail_null(void) {
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

/*
 * 1.4 LIT: match a literal view at the cursor - SNOBOL's implicit string match.
 * Success returns the matched view [old cursor, old cursor + size) and advances
 * the cursor by its size. Failure returns sno_fail() and leaves the subject unchanged.
 * Precondition: subject pointer != NULL (error 10).
 */
void test_lit(void) {
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
void test_find_lit(void) {
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

// 1.8 SUCCEEDED: the exact complement of sno_failed() - the null string succeeds
void test_succeeded(void) {
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

// 1.10.1 SIZE
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

// 1.11 TRIM: trailing POSIX "C" locale whitespace only
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

/*
 * Check order: argument errors first, then failure.
 * A bad argument (a NULL pointer) is a bug in the program and is raised on
 * entry even if an earlier match failed - see test_sno_errors1().
 * A failure view with *valid* arguments is normal flow: it just fails.
 */
void test_fail_first1(void) {
    sno_view_t s;

    // 1.4 LIT
    s = sno_fail();
    assert(sno_failed(sno_lit(&s, sno_bind("A"))));         // failure subject
    assert(sno_failed(sno_lit(&s, sno_null())));            // even the null literal
    s = sno_bind("A");
    assert(sno_failed(sno_lit(&s, sno_fail())));            // failure literal is a value: fails
    assert(view_eq(s, "A"));                                // subject untouched
}

// Precondition violations under &ERRLIMIT (Green Book 10.2.2): with errors allowed, a
// conditionally fatal error is recorded for sno_err_type(), uses one of the limit, and the
// call fails instead of halting. Each line asserts its own error; the count at the end
// proves none was skipped. The real halt and ERRORLEVEL stay with the DOS batch file.
void test_sno_errors1(void) {
    static const char* ops[] = { "PUSHI", "PUSH", "POP", NULL };
    sno_view_t v[8];
    sno_view_t s = sno_bind("X");
    sno_view_t f = sno_fail();

    sno_err_limit(10);                          // exactly one per line below
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

    assert(sno_err_count() == 0);               // all 10 raised, none skipped
    assert(view_eq(s, "X") && sno_failed(f));   // and no subject was moved
    sno_err_limit(0);                           // &ERRLIMIT = 0: errors halt again
}

void test_core1(void) {
    test_char_in();
    test_bind();
    test_bind_list();
    test_fail_null();
    test_lit();
    //test_find_lit();
    test_succeeded();
    test_size();
    test_trim();

    // error handling
    test_fail_first1();
    test_sno_errors1();
    printf("Pass core 1\n");
}
