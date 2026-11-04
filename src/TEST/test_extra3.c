#include "test_extra3.h"
#include "test_view.h"                          // view_eq(), view_same()

#include "../SNO/sno_extra.h"
#include "../SNO/sno_error.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Tests for sno_extra.h (v0.1), Green Book 3.4.2: outside of the SNOBOL
 * semantics in views - character translation of a C string in place.
 * Independent of the other chapters: its own error tests and runner, test_extra3().
 *
 * Two kinds of bad input, as the Green Book (10.2.2):
 *   failure        functions return NULL or sno_fail(), and failure views propagate
 *   precondition   a programmer error: sno_error() raises the Green Book
 *                  error number; under the default &ERRLIMIT = 0 it halts
 *                  with that number as ERRORLEVEL
 *
 * Each test's comment lists its preconditions. test_sno_errors_extra3() raises
 * &ERRLIMIT so every violation is recorded and fails instead of halting,
 * then checks each error number. The real halt and ERRORLEVEL are checked
 * by the DOS batch file.
 */

/*
 * 3.4.2 REPLACE: one-for-one character translation of the C string in buf, in place.
 * Success returns a view of buf, same size as before. Failure returns sno_fail(), buf unchanged.
 * Preconditions: buf != NULL, '\0' within size, from and to != NULL (error 10).
 */
void test_chr_replace(void) {
    char t[32];
    sno_view_t v;

    // Green Book 3.4.2: ones' complement
    memset(t, 'X', sizeof t);
    strcpy(t, "111001");
    v = sno_chr_replace(t, sizeof t, "01", "10");
    assert(view_eq(v, "000110"));
    assert(v.begin == t);                       // a view of buf
    assert(*v.end == '\0');                     // terminator kept, outside view
    assert(t[7] == 'X');                        // nothing past the '\0' touched

    // Green Book 1.10.2: brackets
    strcpy(t, "A(I,J) = A(I,J) + 3");
    assert(view_eq(sno_chr_replace(t, sizeof t, "()", "<>"), "A<I,J> = A<I,J> + 3"));

    // rightmost correspondence holds: FEET -> FOOT
    strcpy(t, "FEET");
    assert(view_eq(sno_chr_replace(t, sizeof t, "EE", "AO"), "FOOT"));

    // repeats in to are fine: punctuation to blanks
    strcpy(t, "A.B,C;D:E");
    assert(view_eq(sno_chr_replace(t, sizeof t, ".,;:", "    "), "A B C D E"));

    // characters not in from are unchanged
    strcpy(t, "PUSHI");
    assert(view_eq(sno_chr_replace(t, sizeof t, "xyz", "XYZ"), "PUSHI"));

    // a swap is one pass, not two: AB -> BA
    strcpy(t, "ABBA");
    assert(view_eq(sno_chr_replace(t, sizeof t, "AB", "BA"), "BAAB"));

    // empty buf: null string at buf
    t[0] = '\0';
    v = sno_chr_replace(t, sizeof t, "A", "B");
    assert(sno_is_null(v) && v.begin == t);

    // exact fit: '\0' in the last byte of size
    strcpy(t, "ABC");
    assert(view_eq(sno_chr_replace(t, 4, "B", "b"), "AbC"));

    // failure: different lengths, "" from or to - buf unchanged
    strcpy(t, "AB");
    assert(sno_failed(sno_chr_replace(t, sizeof t, "AB", "A")));
    assert(sno_failed(sno_chr_replace(t, sizeof t, "A", "AB")));
    assert(sno_failed(sno_chr_replace(t, sizeof t, "", "")));
    assert(sno_failed(sno_chr_replace(t, sizeof t, "", "A")));
    assert(sno_failed(sno_chr_replace(t, sizeof t, "A", "")));
    assert(strcmp(t, "AB") == 0);

    // high-bit chars are ordinary
    strcpy(t, "\xA0\xFF");
    assert(view_eq(sno_chr_replace(t, sizeof t, "\xFF", "\xA0"), "\xA0\xA0"));
}

/*
 * Check order, as test_fail_first_extra1(): a failure with valid arguments
 * just fails - nothing is written.
 */
void test_fail_first_extra3(void) {
    char buf[8];

    // 3.4.2 REPLACE: different lengths or "" just fail, buf unchanged
    strcpy(buf, "AB");
    assert(sno_failed(sno_chr_replace(buf, sizeof buf, "AB", "A")));
    assert(sno_failed(sno_chr_replace(buf, sizeof buf, "", "")));
    assert(strcmp(buf, "AB") == 0);
}

// Precondition violations under &ERRLIMIT (Green Book 10.2.2), as test_sno_errors_extra1():
// each line fails with its own error instead of halting; the count proves none was skipped.
void test_sno_errors_extra3(void) {
    char buf[8];

    sno_err_limit(6);                           // exactly one per line below
    assert(sno_err_type() == SNO_ERR_NONE);     // the limit starts a clean record

    // 3.4.2 REPLACE        6
    assert(F_(sno_chr_replace(NULL, sizeof buf, "A", "B"))                  && sno_err_type() == 10);  // buf NULL
    assert(F_(sno_chr_replace(NULL, sizeof buf, "AB", "A"))                 && sno_err_type() == 10);  // buf NULL beats failure
    assert(F_(sno_chr_replace((char*)memset(buf, 'X', sizeof buf), sizeof buf, "A", "B")) && sno_err_type() == 10); // no '\0' within size
    assert(F_(sno_chr_replace(strcpy(buf, "X"), 0, "A", "B"))               && sno_err_type() == 10);  // size 0: no room for a '\0'
    assert(F_(sno_chr_replace(strcpy(buf, "X"), sizeof buf, NULL, "B"))     && sno_err_type() == 10);  // from NULL
    assert(F_(sno_chr_replace(strcpy(buf, "X"), sizeof buf, "A", NULL))     && sno_err_type() == 10);  // to NULL

    assert(sno_err_count() == 0);               // all 6 raised, none skipped
    sno_err_limit(0);                           // &ERRLIMIT = 0: errors halt again
}

void test_extra3(void) {
    test_chr_replace();

    // error handling
    test_fail_first_extra3();
    //test_sno_errors_extra3();
    printf("Pass extra 3\n");
}
