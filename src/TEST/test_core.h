#ifndef TEST_CORE_H
#define TEST_CORE_H

#include "../SNO/sno_core.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Assumed signatures:
 *   sno_view_t sno_input(FILE *f, char *buf, size_t size);
 *   bool sno_ident(sno_view_t a, sno_view_t b);
 *   sno_view_t sno_trim(sno_view_t v);
 *
 * Conventions under test:
 *   failure view    sno_failed(v)               (sno_fail(), sno_bind(NULL), EOF)
 *   null string     !sno_failed(v), size 0      (sno_bind(""), sno_null())
 */

/* View holds exactly the characters of s (and is not a failure view) */
static int view_eq(sno_view_t v, const char *s) {
    size_t n = strlen(s);
    return !sno_failed(v)
        && (size_t)sno_size(v) == n
        && memcmp(v.begin, s, n) == 0;
}

/* 1.8 Flow Control: failure versus the null string */
static void test_fail() {
    static const char s[] = "X";
    sno_view_t v;

    v = sno_fail();
    assert(sno_failed(v));
    assert(v.begin == NULL && v.end == NULL);   /* the encoding */
    assert(sno_size(v) == 0);

    assert(!sno_failed(sno_null()));    /* null string is success */
    assert(!sno_failed(sno_bind("")));
    assert(!sno_failed(sno_bind("X")));

    v.begin = NULL;                     /* malformed: unsafe to read */
    v.end = s;                          /* so it counts as failed */
    assert(sno_failed(v));
}

static void test_bind() {
    static const char s[] = "TEST";
    static const char e[] = "";
    static const char z[] = "AB\0CD";
    sno_view_t v;

    v = sno_bind(s);
    assert(v.begin == s);               /* a view, not a copy */
    assert(v.end == s + 4);
    assert(sno_size(v) == 4);

    v = sno_bind(e);                    /* null string */
    assert(v.begin == e && v.end == e);
    assert(sno_size(v) == 0);

    v = sno_bind(z);                    /* stops at first NUL */
    assert(v.end == z + 2);
    assert(sno_size(v) == 2);

    v = sno_bind(NULL);                 /* failure */
    assert(sno_failed(v));
    assert(v.begin == NULL && v.end == NULL);
    assert(sno_size(v) == 0);
}

static void test_null() {
    sno_view_t v = sno_null();

    assert(!sno_failed(v));             /* null string, not failure */
    assert(v.begin == v.end);
    assert(sno_size(v) == 0);
}

static void test_size() {
    sno_view_t v = sno_bind("HELLO");

    assert(sno_size(v) == 5);
    v.begin++;                          /* "ELLO" */
    assert(sno_size(v) == 4);
    v.end--;                            /* "ELL" */
    assert(sno_size(v) == 3);
    v.end = v.begin;
    assert(sno_size(v) == 0);
}

static void test_unbind() {
    char buf[16];
    sno_view_t v = sno_bind("HELLO");

    v.begin++;
    v.end--;                            /* "ELL" */

    memset(buf, 'X', sizeof buf);       /* plenty of room */
    assert(sno_unbind(v, buf, sizeof buf) == buf);
    assert(strcmp(buf, "ELL") == 0);
    assert(buf[4] == 'X');

    memset(buf, 'X', sizeof buf);       /* exact fit: 3 chars + NUL */
    sno_unbind(v, buf, 4);
    assert(strcmp(buf, "ELL") == 0);
    assert(buf[4] == 'X');

    memset(buf, 'X', sizeof buf);       /* one short: truncates */
    sno_unbind(v, buf, 3);
    assert(strcmp(buf, "EL") == 0);
    assert(buf[3] == 'X');

    memset(buf, 'X', sizeof buf);       /* room for NUL only */
    assert(sno_unbind(v, buf, 1) == buf);
    assert(buf[0] == '\0' && buf[1] == 'X');

    memset(buf, 'X', sizeof buf);       /* size 0: NULL, writes nothing */
    assert(sno_unbind(v, buf, 0) == NULL);
    assert(buf[0] == 'X');

    memset(buf, 'X', sizeof buf);       /* null string */
    sno_unbind(sno_null(), buf, sizeof buf);
    assert(buf[0] == '\0' && buf[1] == 'X');

    memset(buf, 'X', sizeof buf);       /* failure: NULL, buf untouched */
    assert(sno_unbind(sno_fail(), buf, sizeof buf) == NULL);
    assert(buf[0] == 'X');
}

static void test_output() {
    char buf[16];
    FILE *f = tmpfile();

    assert(f != NULL);

    assert(sno_output(f, sno_bind("HELLO")) == 5);
    assert(sno_output(f, sno_null()) == 0);
    assert(sno_output(f, sno_fail()) == 0);
    assert(!ferror(f));

    rewind(f);
    assert(fread(buf, 1, sizeof buf, f) == 5);  /* no newline appended */
    assert(memcmp(buf, "HELLO", 5) == 0);

    fclose(f);
}

/*
 * sno_input(f, buf, size): view excludes the '\0', but buf holds it,
 * so *v.end == '\0' and buf is a usable C string. One call reads
 * exactly one line; chars beyond size - 1 are discarded.
 */
static void test_input() {
    char area[12];                      /* 8 used, 4 canary */
    sno_view_t v;
    FILE *f = tmpfile();

    assert(f != NULL);

    fputs("ONE\n\nSEVENCH\nTOOLONGLINE\nLAST", f);
    rewind(f);
    memset(area, 'X', sizeof area);

    v = sno_input(f, area, 8);
    assert(view_eq(v, "ONE"));          /* newline stripped */
    assert(v.begin == area);
    assert(*v.end == '\0');             /* terminated, outside view */

    v = sno_input(f, area, 8);          /* empty line: null string */
    assert(v.begin == area && sno_size(v) == 0);
    assert(*v.end == '\0');

    v = sno_input(f, area, 8);          /* exactly fills buffer */
    assert(view_eq(v, "SEVENCH"));
    assert(*v.end == '\0');

    v = sno_input(f, area, 8);          /* long line: first 7 chars */
    assert(view_eq(v, "TOOLONG"));      /* rest of line discarded */
    assert(*v.end == '\0');

    v = sno_input(f, area, 8);          /* last line, no newline */
    assert(view_eq(v, "LAST"));
    assert(*v.end == '\0');

    v = sno_input(f, area, 8);          /* EOF is failure */
    assert(sno_failed(v));
    assert(feof(f));

    v = sno_input(f, area, 8);          /* and stays failure */
    assert(sno_failed(v));

    assert(memcmp(area + 8, "XXXX", 4) == 0);   /* no overrun */

    /* trailing '\n' then EOF: no extra empty line */
    fclose(f);
    f = tmpfile();
    assert(f != NULL);
    fputs("A\n", f);
    rewind(f);
    assert(view_eq(sno_input(f, area, 8), "A"));
    assert(sno_failed(sno_input(f, area, 8)));

    /* blank last line is a real line */
    fclose(f);
    f = tmpfile();
    assert(f != NULL);
    fputs("A\n\n", f);
    rewind(f);
    assert(view_eq(sno_input(f, area, 8), "A"));
    v = sno_input(f, area, 8);
    assert(v.begin == area && sno_size(v) == 0);
    assert(sno_failed(sno_input(f, area, 8)));

    /* bad args: failure, buf untouched */
    rewind(f);
    memset(area, 'X', sizeof area);
    assert(sno_failed(sno_input(NULL, area, 8)));
    assert(sno_failed(sno_input(f, NULL, 8)));
    assert(sno_failed(sno_input(f, area, 0)));
    assert(sno_failed(sno_input(f, area, 1)));    /* no room to progress */
    assert(area[0] == 'X');

    fclose(f);
}

static void test_trim() {
    static const char s[] = "ABC  ";
    sno_view_t v;

    // each of the six POSIX "C" locale spaces on its own
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
    assert(view_eq(sno_trim(sno_null()), ""));

    //not spaces: other control chars and high-bit chars stay.
    // \xA0 is negative in a signed char, so isspace(c) without an
    // (unsigned char) cast is undefined behaviour here
    assert(view_eq(sno_trim(sno_bind("ABC\x1B")), "ABC\x1B"));
    assert(view_eq(sno_trim(sno_bind("ABC\xA0")), "ABC\xA0"));
    assert(view_eq(sno_trim(sno_bind("ABC\xFF")), "ABC\xFF"));

    v = sno_trim(sno_bind(s));
    assert(v.begin == s && v.end == s + 3);     // shrinks, no copy

    assert(sno_failed(sno_trim(sno_fail())));   //failure propagates
}


static void test_char_in() {
    static const char bytes[] = { 'A', '\0', 'B' };
    sno_view_t set = sno_bind("abc");
    sno_view_t v;

    assert(sno_char_in_set('a', set));          /* first */
    assert(sno_char_in_set('c', set));          /* last */
    assert(!sno_char_in_set('d', set));
    assert(!sno_char_in_set('\0', set));        /* terminator not in view */
    assert(!sno_char_in_set('a', sno_null()));  /* empty set */
    assert(!sno_char_in_set('a', sno_fail()));

    v.begin = bytes;                            /* views may hold NUL */
    v.end = bytes + 3;
    assert(sno_char_in_set('\0', v));
    assert(sno_char_in_set('B', v));

    v = sno_bind("\xA0\xFF");                   /* high-bit chars */
    assert(sno_char_in_set('\xA0', v));
    assert(sno_char_in_set('\xFF', v));

    assert(sno_char_in_cstr('a', "abc"));
    assert(sno_char_in_cstr('c', "abc"));
    assert(!sno_char_in_cstr('d', "abc"));
    assert(!sno_char_in_cstr('\0', "abc"));     /* the strchr trap */
    assert(!sno_char_in_cstr('a', ""));
    assert(!sno_char_in_cstr('a', NULL));
    assert(sno_char_in_cstr('\xFF', "\xA0\xFF"));
}

static void test_core() {
    test_fail();
    test_char_in();
    test_bind();
    test_null();
    test_size();
    test_unbind();
    test_output();
    test_input();
    test_trim();
    printf("Pass\n");
}

#endif
