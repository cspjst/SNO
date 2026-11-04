#include "test_extra1.h"
#include "test_view.h"                          // view_eq(), view_same()

#include "../SNO/sno_extra.h"
#include "../SNO/sno_error.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Tests for sno_extra.h (v0.1), Green Book 1.1 - 1.5: outside of the SNOBOL
 * semantics in views - reading streams and writing C strings into a caller's buf/size.
 * Independent of the other chapters: its own error tests and runner, test_extra1().
 *
 * Two kinds of bad input, as the Green Book (10.2.2):
 *   failure        functions return NULL or sno_fail(), and failure views propagate
 *   precondition   a programmer error: sno_error() raises the Green Book
 *                  error number; under the default &ERRLIMIT = 0 it halts
 *                  with that number as ERRORLEVEL
 *
 * Each test's comment lists its preconditions. test_sno_errors_extra1() raises
 * &ERRLIMIT so every violation is recorded and fails instead of halting,
 * then checks each error number. The real halt and ERRORLEVEL are checked
 * by the DOS batch file.
 */

/*
 * 1.1 sno_unbind(v, buf, size): copies at most size - 1 chars and always
 * terminates. Returns NULL, writing nothing, for size == 0 or a
 * failure view. Each case checks the byte after the expected write.
 * Precondition: buf != NULL
 */
void test_unbind(void) {
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
void test_output(void) {
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
void test_input(void) {
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
void test_str_replace(void) {
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
 * Check order, as test_fail_first1(): a failure with valid arguments
 * just fails - nothing is written, and no error 15 as nothing is formed.
 */
void test_fail_first_extra1(void) {
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
}

// Precondition violations under &ERRLIMIT (Green Book 10.2.2), as test_sno_errors1():
// each line fails with its own error instead of halting; the count proves none was skipped.
void test_sno_errors_extra1(void) {
    const char* m[] = { "$0", NULL };
    const char* r[] = { "axbx", NULL };
    const char* r2[] = { "axbx", "cx", NULL };
    char buf[8];
    sno_view_t s = sno_bind("X");
    sno_view_t f = sno_fail();
    FILE* t = tmpfile();                        // a good stream
    FILE* w = fopen("errors.tmp", "w");         // write-only: reading it is a read error

    assert(t != NULL && w != NULL);
    sno_err_limit(15);                          // exactly one per line below
    assert(sno_err_type() == SNO_ERR_NONE);     // the limit starts a clean record

    // 1.1 unbind           2
    assert(sno_unbind(s, NULL, sizeof buf) == NULL                          && sno_err_type() == 10);  // buf NULL
    assert(sno_unbind(f, NULL, sizeof buf) == NULL                          && sno_err_type() == 10);  // buf NULL beats failure

    // 1.3.4 input / output 9
    assert(F_(sno_output(NULL, s))                                          && sno_err_type() == 12);  // f NULL
    assert(F_(sno_output(NULL, f))                                          && sno_err_type() == 12);  // f NULL beats failure
    assert(F_(sno_input(NULL, buf, sizeof buf))                             && sno_err_type() == 12);  // f NULL
    assert(F_(sno_input(t, NULL, sizeof buf))                               && sno_err_type() == 10);  // buf NULL
    assert(F_(sno_input(t, buf, 1))                                         && sno_err_type() == 10);  // size < 2
    assert(F_(sno_input(t, buf, 0))                                         && sno_err_type() == 10);  // size < 2
    assert(F_(sno_input(w, buf, sizeof buf))                                && sno_err_type() == 11);  // read error

    // 1.5 str_replace      15
    assert(F_(sno_str_replace(NULL, sizeof buf, m, r))                      && sno_err_type() == 10);  // buf NULL
    assert(F_(sno_str_replace(NULL, sizeof buf, m, r2))                     && sno_err_type() == 10);  // buf NULL beats failure
    assert(F_(sno_str_replace((char*)memset(buf, 'X', sizeof buf), sizeof buf, m, r)) && sno_err_type() == 10); // no '\0' within size
    assert(F_(sno_str_replace(strcpy(buf, "X"), sizeof buf, NULL, r))       && sno_err_type() == 10);  // from NULL
    assert(F_(sno_str_replace(strcpy(buf, "X"), sizeof buf, m, NULL))       && sno_err_type() == 10);  // to NULL
    assert(F_(sno_str_replace(strcpy(buf, "$0$0"), sizeof buf, m, r))       && sno_err_type() == 15);  // result too big
    assert(strcmp(buf, "$0$0") == 0);           // an overflow writes nothing

    assert(sno_err_count() == 0);               // all 15 raised, none skipped
    sno_err_limit(0);                           // &ERRLIMIT = 0: errors halt again

    fclose(t);
    fclose(w);
    remove("errors.tmp");
}

void test_extra1(void) {
    test_unbind();
    test_output();
    test_input();
    test_str_replace();

    // error handling
    test_fail_first_extra1();
    //test_sno_errors_extra1();
    printf("Pass extra 1\n");
}
