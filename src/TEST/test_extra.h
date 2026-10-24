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

/*
 * Check order, as test_fail_first(): a failure view with valid
 * arguments just fails - for unbind, NULL and nothing written.
 */
static void test_fail_first_extra() {
    char buf[8];

    memset(buf, 'X', sizeof buf);
    assert(sno_unbind(sno_fail(), buf, sizeof buf) == NULL);
    assert(sno_unbind(sno_fail(), buf, 0) == NULL);
    assert(buf[0] == 'X');
}

// Precondition violations, as test_sno_halt(): uncomment ONE line at a time;
// a pass is a halt with ERRORLEVEL = that error, not "Pass".
static void test_sno_halt_extra() {
    char buf[8];
    sno_view_t s = sno_bind("X");
    sno_view_t f = sno_fail();

    // 1.1 unbind
    //sno_unbind(s, NULL, sizeof buf);                    // 10  buf NULL
    //sno_unbind(f, NULL, sizeof buf);                    // 10  buf NULL beats failure

    // 1.3.4 input / output
    //sno_output(NULL, s);                                // 12  f NULL
    //sno_input(NULL, buf, sizeof buf);                   // 12  f NULL
    //sno_input(tmpfile(), NULL, sizeof buf);             // 10  buf NULL
    //sno_input(tmpfile(), buf, 1);                       // 10  size < 2
    //sno_input(tmpfile(), buf, 0);                       // 10  size < 2
    //sno_input(fopen("halt.tmp", "w"), buf, sizeof buf); // 11  read error

    (void)buf;                                          // quiet when all are commented out
    (void)s;
    (void)f;
}

static void test_extra() {
    test_unbind();
    test_output();
    test_input();

    test_fail_first_extra();
    //test_sno_halt_extra();
    printf("Pass extra\n");
}

#endif
