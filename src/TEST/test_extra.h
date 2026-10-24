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
 * 1.5 C PROXY: expand markers from[i] -> to[i] in place, NULL-terminated lists.
 * Success returns a view of buf, buf terminated. Failure returns sno_fail(), buf unchanged.
 * Preconditions: buf != NULL, '\0' within size, from and to != NULL (error 10);
 * result fits (error 15).
 */
static void test_proxy() {
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
    v = sno_proxy(t, sizeof t, dollar, frame);
    assert(view_eq(v, "sub sp,3*2"));
    assert(v.begin == t);                       // a view of buf
    assert(*v.end == '\0');                     // terminated, outside view
    assert(t[12] == 'X');                       // nothing past the old '\0' touched

    // several markers, repeated, adjacent; result grows
    strcpy(t, "mov $0,$1\nadd $0,$0");
    assert(view_eq(sno_proxy(t, sizeof t, dollar, frame), "mov ax,3\nadd ax,ax"));
    strcpy(t, "$0$1$0");
    assert(view_eq(sno_proxy(t, sizeof t, dollar, frame), "ax3ax"));

    // markers of any length
    strcpy(t, "Hi {{name}}!");
    assert(view_eq(sno_proxy(t, sizeof t, name, x), "Hi X!"));

    // "" deletes the marker; result shrinks
    strcpy(t, "A$0B");
    assert(view_eq(sno_proxy(t, sizeof t, dollar, blanks), "AB"));

    // no markers in buf: unchanged
    strcpy(t, "push bp");
    assert(view_eq(sno_proxy(t, sizeof t, dollar, frame), "push bp"));

    // text matching no marker is copied: A86's current location, a marker cut short
    strcpy(t, "jmp $");
    assert(view_eq(sno_proxy(t, sizeof t, dollar, frame), "jmp $"));
    strcpy(t, "$x $2 $");
    assert(view_eq(sno_proxy(t, sizeof t, dollar, frame), "$x $2 $"));

    // first marker in the list wins: list "$10" before "$1"
    strcpy(t, "$10 $1");
    assert(view_eq(sno_proxy(t, sizeof t, ten, pair), "TEN ONE"));
    strcpy(t, "$10 $1");
    assert(view_eq(sno_proxy(t, sizeof t, one, pair), "TEN0 TEN"));

    // single pass: a marker inside a replacement is not expanded
    strcpy(t, "$0");
    assert(view_eq(sno_proxy(t, sizeof t, dollar, again), "$1"));

    // empty buf: null string at buf
    t[0] = '\0';
    v = sno_proxy(t, sizeof t, dollar, frame);
    assert(sno_is_null(v) && v.begin == t);

    // exact fit: result size + 1, growing in place
    memset(t, 'X', sizeof t);
    strcpy(t, "$0$0");                          // "axax" needs 5
    v = sno_proxy(t, 5, dollar, frame);
    assert(view_eq(v, "axax"));
    assert(t[4] == '\0' && t[5] == 'X');

    // failure: different lengths, empty from, "" marker - buf unchanged
    strcpy(t, "$0$1");
    assert(sno_failed(sno_proxy(t, sizeof t, dollar, short_to)));
    assert(sno_failed(sno_proxy(t, sizeof t, short_to, frame)));
    assert(sno_failed(sno_proxy(t, sizeof t, none, none)));
    assert(sno_failed(sno_proxy(t, sizeof t, blank, x)));
    assert(strcmp(t, "$0$1") == 0);
}

/*
 * 3.4.2 REPLACE: one-for-one character translation of the C string in buf, in place.
 * Success returns a view of buf, same size as before. Failure returns sno_fail(), buf unchanged.
 * Preconditions: buf != NULL, '\0' within size (error 10).
 */
static void test_replace() {
    static const char nul[] = { '\0' };
    char t[32];
    sno_view_t v, from;

    // Green Book 3.4.2: ones' complement
    memset(t, 'X', sizeof t);
    strcpy(t, "111001");
    v = sno_replace(t, sizeof t, sno_bind("01"), sno_bind("10"));
    assert(view_eq(v, "000110"));
    assert(v.begin == t);                       // a view of buf
    assert(*v.end == '\0');                     // terminator kept, outside view
    assert(t[7] == 'X');                        // nothing past the '\0' touched

    // Green Book 1.10.2: brackets
    strcpy(t, "A(I,J) = A(I,J) + 3");
    assert(view_eq(sno_replace(t, sizeof t, sno_bind("()"), sno_bind("<>")), "A<I,J> = A<I,J> + 3"));

    // rightmost correspondence holds: FEET -> FOOT
    strcpy(t, "FEET");
    assert(view_eq(sno_replace(t, sizeof t, sno_bind("EE"), sno_bind("AO")), "FOOT"));

    // repeats in to are fine: punctuation to blanks
    strcpy(t, "A.B,C;D:E");
    assert(view_eq(sno_replace(t, sizeof t, sno_bind(".,;:"), sno_bind("    ")), "A B C D E"));

    // characters not in from are unchanged
    strcpy(t, "PUSHI");
    assert(view_eq(sno_replace(t, sizeof t, sno_bind("xyz"), sno_bind("XYZ")), "PUSHI"));

    // empty buf: null string at buf
    t[0] = '\0';
    v = sno_replace(t, sizeof t, sno_bind("A"), sno_bind("B"));
    assert(sno_is_null(v) && v.begin == t);

    // exact fit: '\0' in the last byte of size
    strcpy(t, "ABC");
    assert(view_eq(sno_replace(t, 4, sno_bind("B"), sno_bind("b")), "AbC"));

    // failure: different sizes, null from or to, failure views - buf unchanged
    strcpy(t, "AB");
    assert(sno_failed(sno_replace(t, sizeof t, sno_bind("AB"), sno_bind("A"))));
    assert(sno_failed(sno_replace(t, sizeof t, sno_null(), sno_null())));
    assert(sno_failed(sno_replace(t, sizeof t, sno_null(), sno_bind("A"))));
    assert(sno_failed(sno_replace(t, sizeof t, sno_bind("A"), sno_null())));
    assert(sno_failed(sno_replace(t, sizeof t, sno_fail(), sno_bind("B"))));
    assert(sno_failed(sno_replace(t, sizeof t, sno_bind("A"), sno_fail())));
    assert(strcmp(t, "AB") == 0);

    // '\0' in to: the view keeps the size, the C string stops early
    from.begin = nul;
    from.end = nul + 1;                         // { '\0' }
    strcpy(t, "A-B");
    v = sno_replace(t, sizeof t, sno_bind("-"), from);
    assert(sno_size(v) == 3);
    assert(t[1] == '\0' && t[2] == 'B' && t[3] == '\0');
    assert(strlen(t) == 1);

    // '\0' in from never matches: the terminator is not translated
    strcpy(t, "AB");
    v = sno_replace(t, sizeof t, from, sno_bind("-"));
    assert(view_eq(v, "AB"));
    assert(t[2] == '\0');

    // high-bit chars are ordinary
    strcpy(t, "\xA0\xFF");
    assert(view_eq(sno_replace(t, sizeof t, sno_bind("\xFF"), sno_bind("\xA0")), "\xA0\xA0"));
}

/*
 * Check order, as test_fail_first(): a failure with valid arguments
 * just fails - nothing is written, and no error 15 as nothing is formed.
 */
static void test_fail_first_extra() {
    const char* m[] = { "$0", NULL };
    const char* r2[] = { "axbx", "cx", NULL };
    char buf[8];

    // 1.1 unbind: NULL, nothing written
    memset(buf, 'X', sizeof buf);
    assert(sno_unbind(sno_fail(), buf, sizeof buf) == NULL);
    assert(sno_unbind(sno_fail(), buf, 0) == NULL);
    assert(buf[0] == 'X');

    // 1.5 proxy: lists of different lengths fail before the result could overflow
    strcpy(buf, "$0$0$0");
    assert(sno_failed(sno_proxy(buf, sizeof buf, m, r2)));
    assert(strcmp(buf, "$0$0$0") == 0);

    // 3.4.2 REPLACE: a failure view as from or to just fails
    strcpy(buf, "AB");
    assert(sno_failed(sno_replace(buf, sizeof buf, sno_fail(), sno_bind("B"))));
    assert(sno_failed(sno_replace(buf, sizeof buf, sno_bind("A"), sno_fail())));
    assert(strcmp(buf, "AB") == 0);
}

// Precondition violations, as test_sno_halt(): uncomment ONE line at a time;
// a pass is a halt with ERRORLEVEL = that error, not "Pass".
static void test_sno_halt_extra() {
    const char* m[] = { "$0", NULL };
    const char* r[] = { "axbx", NULL };
    const char* r2[] = { "axbx", "cx", NULL };
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

    // 1.5 proxy
    //sno_proxy(NULL, sizeof buf, m, r);                  // 10  buf NULL
    //sno_proxy(NULL, sizeof buf, m, r2);                 // 10  buf NULL beats failure
    //sno_proxy(memset(buf, 'X', sizeof buf), sizeof buf, m, r); // 10  no '\0' within size
    //sno_proxy(strcpy(buf, "X"), sizeof buf, NULL, r);   // 10  from NULL
    //sno_proxy(strcpy(buf, "X"), sizeof buf, m, NULL);   // 10  to NULL
    //sno_proxy(strcpy(buf, "$0$0"), sizeof buf, m, r);   // 15  result too big

    // 3.4.2 REPLACE
    //sno_replace(NULL, sizeof buf, s, s);                // 10  buf NULL
    //sno_replace(NULL, sizeof buf, f, s);                // 10  buf NULL beats failure
    //sno_replace(memset(buf, 'X', sizeof buf), sizeof buf, s, s); // 10  no '\0' within size
    //sno_replace(strcpy(buf, "X"), 0, s, s);             // 10  size 0: no room for a '\0'

    (void)m;                                            // quiet when all are commented out
    (void)r;
    (void)r2;
    (void)buf;
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
