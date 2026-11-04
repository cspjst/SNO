#include "test_core2.h"
#include "test_view.h"                          // view_eq(), view_same()

#include "../SNO/sno_core.h"
#include "../SNO/sno_error.h"

#include <stdio.h>
#include <assert.h>

/*
 * Tests for sno_core.h (v0.4), Green Book 2.8 - 2.12:
 * Chapter 2 - Pattern Matching.
 * Independent of the other chapters: its own error tests and runner, test_core2().
 *
 * Two kinds of bad input, as the Green Book (10.2.2):
 *   failure        the text doesn't match: functions return false or
 *                  sno_fail(), and failure views propagate
 *   precondition   a programmer error: sno_error() raises the Green Book
 *                  error number; under the default &ERRLIMIT = 0 it halts
 *                  with that number as ERRORLEVEL
 *
 * Each test's comment lists its preconditions. test_sno_errors2() raises
 * &ERRLIMIT so every violation is recorded and fails instead of halting,
 * then checks each error number. The real halt and
 * ERRORLEVEL are checked by the DOS batch file.
 */

/*
 * 2.8 LEN: match exactly n characters, whatever they are.
 * Success returns the matched view [old cursor, old cursor + n)
 * and advances begin by n; end is never moved.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL; n >= 0
 * (Green Book error 14, NEGATIVE NUMBER IN ILLEGAL CONTEXT).
 */
void test_len(void) {
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
 * 2.9 BREAK: match up to, but not including, the first break character.
 * Success returns [old cursor, break character) and moves the cursor to the
 * break character; at a break character it is a null string at the cursor.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL (error 10); set not the null
 * string (error 4, NULL STRING IN ILLEGAL CONTEXT).
 */
void test_break(void) {
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
 * 2.10 ANY and NOTANY: match exactly one character.
 * Success returns the one-character match [old cursor, new cursor),
 * advances begin by one and leaves end alone.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL; set not the null string
 * (Green Book error 4, NULL STRING IN ILLEGAL CONTEXT).
 */
void test_any_notany(void) {
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
 * 2.11 RTAB: match from the cursor up to, but not including, the nth
 * character from the end. RTAB(0) is REM.
 * Success returns the matched view [old cursor, end - n) and moves
 * begin to end - n; end is never moved.
 * Failure returns sno_fail() and leaves the subject unchanged.
 * Preconditions: subject pointer != NULL (error 10); n >= 0
 * (error 14, NEGATIVE NUMBER IN ILLEGAL CONTEXT).
 */
void test_rtab(void) {
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
void test_rem(void) {
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
void test_tab(void) {
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
void test_rpos(void) {
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

/*
 * Check order: argument errors first, then failure.
 * A bad argument (n < 0, null set, NULL pointer) is a bug in the program and
 * is raised on entry even if an earlier match failed - see test_sno_errors2().
 * A failure view with *valid* arguments is normal flow: it just fails.
 */
void test_fail_first2(void) {
    sno_view_t s;

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
void test_sno_errors2(void) {
    sno_view_t s = sno_bind("X");
    sno_view_t f = sno_fail();

    sno_err_limit(22);                          // exactly one per line below
    assert(sno_err_type() == SNO_ERR_NONE);     // the limit starts a clean record

    // 2.8 LEN              3
    assert(F_(sno_len(NULL, 1))                 && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_len(&s, -1))                  && sno_err_type() == 14);  // n < 0
    assert(F_(sno_len(&f, -1))                  && sno_err_type() == 14);  // n < 0 beats failure

    // 2.9 BREAK            6
    assert(F_(sno_break(NULL, sno_bind(".")))   && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_break(&s, sno_null()))        && sno_err_type() == 4);   // null set
    assert(F_(sno_break(&f, sno_null()))        && sno_err_type() == 4);   // null set beats failure

    // 2.10 ANY / NOTANY    12
    assert(F_(sno_any(NULL, sno_bind("A")))     && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_any(&s, sno_null()))          && sno_err_type() == 4);   // null set
    assert(F_(sno_any(&f, sno_null()))          && sno_err_type() == 4);   // null set beats failure
    assert(F_(sno_notany(NULL, sno_bind("A")))  && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_notany(&s, sno_null()))       && sno_err_type() == 4);   // null set
    assert(F_(sno_notany(&f, sno_null()))       && sno_err_type() == 4);   // null set beats failure

    // 2.11 TAB / RTAB / REM    19
    assert(F_(sno_tab(NULL, 0))                 && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_tab(&s, -1))                  && sno_err_type() == 14);  // n < 0
    assert(F_(sno_tab(&f, -1))                  && sno_err_type() == 14);  // n < 0 beats failure
    assert(F_(sno_rtab(NULL, 0))                && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_rtab(&s, -1))                 && sno_err_type() == 14);  // n < 0
    assert(F_(sno_rtab(&f, -1))                 && sno_err_type() == 14);  // n < 0 beats failure
    assert(F_(sno_rem(NULL))                    && sno_err_type() == 10);  // subject pointer NULL, via RTAB

    // 2.12 RPOS            22
    assert(F_(sno_rpos(NULL, 0))                && sno_err_type() == 10);  // subject pointer NULL
    assert(F_(sno_rpos(&s, -1))                 && sno_err_type() == 14);  // n < 0
    assert(F_(sno_rpos(&f, -1))                 && sno_err_type() == 14);  // n < 0 beats failure

    assert(sno_err_count() == 0);               // all 22 raised, none skipped
    assert(view_eq(s, "X") && sno_failed(f));   // and no subject was moved
    sno_err_limit(0);                           // &ERRLIMIT = 0: errors halt again
}

void test_core2(void) {
    test_len();
    test_break();
    test_any_notany();
    test_rtab();
    test_rem();
    test_tab();
    test_rpos();

    // error handling
    test_fail_first2();
    test_sno_errors2();
    printf("Pass core 2\n");
}
