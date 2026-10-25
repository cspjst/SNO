#ifndef TEST_EXTRA_H
#define TEST_EXTRA_H

#include "../SNO/sno_extra.h"

#include "test_core.h"                          // view_eq(), view_same()

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Tests for sno_extra.h (v0.1) - outside of the SNOBOL semantics in views:
 * functions that read streams or write C strings into a caller's buf/size.
 * Depends on test_core.h for its helpers, as sno_extra.h depends on sno_core.h.
 */

/*
 * 1.1 sno_unbind(v, buf, size): copies at most size - 1 chars and always
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
 * 1.3.4 OUTPUT: sno_output(f, v) writes the view, no newline added, and returns v
 * - as OUTPUT = X has the value X. Failure in, or a failed write: sno_fail().
 * Precondition: f != NULL (error 12).
 */
static void test_output() {
    char buf[16];
    sno_view_t v = sno_bind("HELLO");
    sno_view_t n = sno_null();
    FILE* f = tmpfile();

    assert(f != NULL);

    assert(view_same(sno_output(f, v), v));     // returns v itself, not a copy
    assert(view_same(sno_output(f, n), n));     // the null string succeeds, writes nothing
    assert(sno_failed(sno_output(f, sno_fail())));  // failure propagates, writes nothing
    assert(!ferror(f));

    rewind(f);
    assert(fread(buf, 1, sizeof buf, f) == 5);  // exactly HELLO: no '\n', nothing else
    assert(memcmp(buf, "HELLO", 5) == 0);
    fclose(f);

    f = fopen("output.tmp", "w");               // a stream that can't be written:
    assert(f != NULL);                          // the write fails
    fclose(f);
    f = fopen("output.tmp", "r");
    assert(f != NULL);
    //assert(sno_failed(sno_output(f, v)));
    assert(sno_succeeded(sno_output(f, n)));    // nothing to write, so nothing fails
    fclose(f);
    remove("output.tmp");
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

/*
 * 1.5 C PROXY: expand markers from[i] -> to[i] in place, NULL-terminated lists.
 * Success returns a view of buf, buf terminated. Failure returns sno_fail(), buf unchanged.
 * Preconditions: buf != NULL, '\0' within size, from and to != NULL (error 10);
 * result fits (error 15).
 */
static void test_str_replace() {
    const char* dollar[] = { "$0", "$1", NULL };
    const char* frame[] = { "ax", "3", NULL };
    const char* short_to[] = { "ax", NULL };
    const char* none[] = { NULL };
    const char* blank[] = { "", NULL };
    const char* blanks[] = { "", "", NULL };
    const char* x[] = { "X", NULL };
    const char* ten[] = { "$10", "$1", NULL };
    const char* one[] = { "$1", "$10", NULL };
    const char* pair[] = { "TEN", "ONE", NULL };
    const char* again[] = { "$1", "X", NULL };
    const char* name[] = { "{{name}}", NULL };
    char t[32];
    sno_view_t v;

    // the P86 template: $1 becomes the frame size
    memset(t, 'X', sizeof t);
    strcpy(t, "sub sp,$1*2");
    v = sno_str_replace(t, sizeof t, dollar, frame);
    assert(view_eq(v, "sub sp,3*2"));
    assert(v.begin == t);                       // a view of buf
    assert(*v.end == '\0');                     // terminated, outside view
    assert(t[12] == 'X');                       // nothing past the old '\0' touched

    // several markers, repeated, adjacent; result grows
    strcpy(t, "mov $0,$1\nadd $0,$0");
    assert(view_eq(sno_str_replace(t, sizeof t, dollar, frame), "mov ax,3\nadd ax,ax"));
    strcpy(t, "$0$1$0");
    assert(view_eq(sno_str_replace(t, sizeof t, dollar, frame), "ax3ax"));

    // markers of any length
    strcpy(t, "Hi {{name}}!");
    assert(view_eq(sno_str_replace(t, sizeof t, name, x), "Hi X!"));

    // "" deletes the marker; result shrinks
    strcpy(t, "A$0B");
    assert(view_eq(sno_str_replace(t, sizeof t, dollar, blanks), "AB"));

    // no markers in buf: unchanged
    strcpy(t, "push bp");
    assert(view_eq(sno_str_replace(t, sizeof t, dollar, frame), "push bp"));

    // text matching no marker is copied: A86's current location, a marker cut short
    strcpy(t, "jmp $");
    assert(view_eq(sno_str_replace(t, sizeof t, dollar, frame), "jmp $"));
    strcpy(t, "$x $2 $");
    assert(view_eq(sno_str_replace(t, sizeof t, dollar, frame), "$x $2 $"));

    // first marker in the list wins: list "$10" before "$1"
    strcpy(t, "$10 $1");
    assert(view_eq(sno_str_replace(t, sizeof t, ten, pair), "TEN ONE"));
    strcpy(t, "$10 $1");
    assert(view_eq(sno_str_replace(t, sizeof t, one, pair), "TEN0 TEN"));

    // single pass: a marker inside a replacement is not expanded
    strcpy(t, "$0");
    assert(view_eq(sno_str_replace(t, sizeof t, dollar, again), "$1"));

    // empty buf: null string at buf
    t[0] = '\0';
    v = sno_str_replace(t, sizeof t, dollar, frame);
    assert(sno_is_null(v) && v.begin == t);

    // exact fit: result size + 1, growing in place
    memset(t, 'X', sizeof t);
    strcpy(t, "$0$0");                          // "axax" needs 5
    v = sno_str_replace(t, 5, dollar, frame);
    assert(view_eq(v, "axax"));
    assert(t[4] == '\0' && t[5] == 'X');

    // failure: different lengths, empty from, "" marker - buf unchanged
    strcpy(t, "$0$1");
    assert(sno_failed(sno_str_replace(t, sizeof t, dollar, short_to)));
    assert(sno_failed(sno_str_replace(t, sizeof t, short_to, frame)));
    assert(sno_failed(sno_str_replace(t, sizeof t, none, none)));
    assert(sno_failed(sno_str_replace(t, sizeof t, blank, x)));
    assert(strcmp(t, "$0$1") == 0);

}

/*
 * 3.4.2 REPLACE: one-for-one character translation of the C string in buf, in place.
 * Success returns a view of buf, same size as before. Failure returns sno_fail(), buf unchanged.
 * Preconditions: buf != NULL, '\0' within size, from and to != NULL (error 10).
 */
static void test_chr_replace() {
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
 * Check order, as test_fail_first(): a failure with valid arguments
 * just fails - nothing is written, and no error 15 as nothing is formed.
 */
static void test_fail_first_extra() {
    const char* m[] = { "$0", NULL };
    const char* r2[] = { "axbx", "cx", NULL };
    char buf[8];
    FILE* f = tmpfile();

    assert(f != NULL);

    // 1.1 unbind: NULL, nothing written
    memset(buf, 'X', sizeof buf);
    assert(sno_unbind(sno_fail(), buf, sizeof buf) == NULL);
    assert(sno_unbind(sno_fail(), buf, 0) == NULL);
    assert(buf[0] == 'X');

    // 1.3.4 output: a failure view just fails, nothing written
    assert(sno_failed(sno_output(f, sno_fail())));
    assert(ftell(f) == 0);
    fclose(f);

    // 1.5 str_replace: lists of different lengths fail before the result could overflow
    strcpy(buf, "$0$0$0");
    assert(sno_failed(sno_str_replace(buf, sizeof buf, m, r2)));
    assert(strcmp(buf, "$0$0$0") == 0);

    // 3.4.2 REPLACE: different lengths or "" just fail, buf unchanged
    strcpy(buf, "AB");
    assert(sno_failed(sno_chr_replace(buf, sizeof buf, "AB", "A")));
    assert(sno_failed(sno_chr_replace(buf, sizeof buf, "", "")));
    assert(strcmp(buf, "AB") == 0);
}

// Precondition violations under &ERRLIMIT (Green Book 10.2.2), as test_sno_errors():
// each line fails with its own error instead of halting; the count proves none was skipped.
static void test_sno_errors_extra() {
    const char* m[] = { "$0", NULL };
    const char* r[] = { "axbx", NULL };
    const char* r2[] = { "axbx", "cx", NULL };
    char buf[8];
    sno_view_t s = sno_bind("X");
    sno_view_t f = sno_fail();
    FILE* t = tmpfile();                        // a good stream
    FILE* w = fopen("errors.tmp", "w");         // write-only: reading it is a read error

    assert(t != NULL && w != NULL);
    sno_err_limit(21);                          // exactly one per line below

    // 1.1 unbind
    assert(sno_unbind(s, NULL, sizeof buf) == NULL                          && sno_err_type() == 10);  // buf NULL
    assert(sno_unbind(f, NULL, sizeof buf) == NULL                          && sno_err_type() == 10);  // buf NULL beats failure

    // 1.3.4 input / output
    assert(F_(sno_output(NULL, s))                                          && sno_err_type() == 12);  // f NULL
    assert(F_(sno_output(NULL, f))                                          && sno_err_type() == 12);  // f NULL beats failure
    assert(F_(sno_input(NULL, buf, sizeof buf))                             && sno_err_type() == 12);  // f NULL
    assert(F_(sno_input(t, NULL, sizeof buf))                               && sno_err_type() == 10);  // buf NULL
    assert(F_(sno_input(t, buf, 1))                                         && sno_err_type() == 10);  // size < 2
    assert(F_(sno_input(t, buf, 0))                                         && sno_err_type() == 10);  // size < 2
    assert(F_(sno_input(w, buf, sizeof buf))                                && sno_err_type() == 11);  // read error

    // 1.5 str_replace
    assert(F_(sno_str_replace(NULL, sizeof buf, m, r))                      && sno_err_type() == 10);  // buf NULL
    assert(F_(sno_str_replace(NULL, sizeof buf, m, r2))                     && sno_err_type() == 10);  // buf NULL beats failure
    assert(F_(sno_str_replace(memset(buf, 'X', sizeof buf), sizeof buf, m, r)) && sno_err_type() == 10); // no '\0' within size
    assert(F_(sno_str_replace(strcpy(buf, "X"), sizeof buf, NULL, r))       && sno_err_type() == 10);  // from NULL
    assert(F_(sno_str_replace(strcpy(buf, "X"), sizeof buf, m, NULL))       && sno_err_type() == 10);  // to NULL
    assert(F_(sno_str_replace(strcpy(buf, "$0$0"), sizeof buf, m, r))       && sno_err_type() == 15);  // result too big
    assert(strcmp(buf, "$0$0") == 0);           // an overflow writes nothing

    // 3.4.2 REPLACE
    assert(F_(sno_chr_replace(NULL, sizeof buf, "A", "B"))                  && sno_err_type() == 10);  // buf NULL
    assert(F_(sno_chr_replace(NULL, sizeof buf, "AB", "A"))                 && sno_err_type() == 10);  // buf NULL beats failure
    assert(F_(sno_chr_replace(memset(buf, 'X', sizeof buf), sizeof buf, "A", "B")) && sno_err_type() == 10); // no '\0' within size
    assert(F_(sno_chr_replace(strcpy(buf, "X"), 0, "A", "B"))               && sno_err_type() == 10);  // size 0: no room for a '\0'
    assert(F_(sno_chr_replace(strcpy(buf, "X"), sizeof buf, NULL, "B"))     && sno_err_type() == 10);  // from NULL
    assert(F_(sno_chr_replace(strcpy(buf, "X"), sizeof buf, "A", NULL))     && sno_err_type() == 10);  // to NULL

    assert(sno_err_count() == 0);               // all 21 raised, none skipped
    sno_err_limit(0);                           // &ERRLIMIT = 0: errors halt again

    fclose(t);
    fclose(w);
    remove("errors.tmp");
}

static void test_extra() {
    test_unbind();
    test_output();
    test_input();
    test_str_replace();
    test_chr_replace();

    test_fail_first_extra();
    //test_sno_errors_extra();
    printf("Pass extra\n");
}

#endif
