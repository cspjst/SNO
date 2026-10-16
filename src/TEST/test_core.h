#ifndef TEST_CORE_H
#define TEST_CORE_H

#include "../SNO/sno_core.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

static void test_char_in() {
    static const char bytes[] = { 'A', '\0', 'B' };
    sno_view_t set = sno_bind("abc");
    sno_view_t v;

    assert(sno_char_in_set('a', set));          /* first */
    assert(sno_char_in_set('c', set));          /* last */
    assert(!sno_char_in_set('d', set));
    assert(!sno_char_in_set('\0', set));        /* terminator not in view */
    assert(!sno_char_in_set('a', sno_null()));  /* empty set */
    assert(!sno_char_in_set('a', sno_bind(NULL)));

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

static void test_bind() {
    static const char s[] = "TEST";
    static const char e[] = "";
    static const char z[] = "AB\0CD";
    sno_view_t v;

    v = sno_bind("TEST");
    assert(v.begin && v.end && sno_size(v) == 4);

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
    assert(v.begin == NULL && v.end == NULL);
    assert(sno_size(v) == 0);
}

static void test_null() {
    sno_view_t v = sno_null();

    assert(v.begin != NULL);            /* null string, not failure */
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

    memset(buf, 'X', sizeof buf);       /* size 0: NULL, writes nothing */
    assert(sno_unbind(v, buf, 0) == NULL);
    assert(buf[0] == 'X');
}

static void test_core() {
    test_char_in();
    test_bind();
    test_null();
    test_size();
    test_unbind();
    printf("Pass\n");
}

#endif
