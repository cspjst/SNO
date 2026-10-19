#include <stdio.h>

#include "sno_core.h"
#include "sno_types.h"
#include "sno_constants.h"

// 1.3.1
static const char SNO_NULL_STRING[] = "";

bool sno_char_in_set(char c, sno_view_t v) {
    if(sno_failed(v)) return false;
    sno_cursor_t p = v.begin;
    while (p < v.end && *p != c) p++;
    return p != v.end;
}

bool sno_char_in_cstr(char c, const char* cstr)  {
    if(!cstr) return false;
    sno_cursor_t p = cstr;
    while (*p && *p != c) p++;
    return *p != '\0';
}

sno_view_t sno_bind(const char* cstr) {
    sno_view_t v;
    if (!cstr) return sno_fail();
    v.begin = v.end = cstr;
    if(cstr) while(*v.end) v.end++;
    return v;
}

char* sno_unbind(sno_view_t v, char* buf, size_t size) {
    if(sno_failed(v) || !buf || !size) return NULL;
    char* p = buf;
    char* buf_end = p + size - 1;
    while(v.begin < v.end && p < buf_end) *p++ = *v.begin++;
    *p = '\0';
    return buf;
}

sno_view_t sno_null() {
    sno_view_t v;
    v.begin = v.end = SNO_NULL_STRING;
    return v;
}

sno_view_t sno_fail() {
    sno_view_t v;
    v.begin = v.end = NULL;
    return v;
}

int sno_output(FILE* f, sno_view_t v) {
    if(!f || sno_failed(v)) return 0;
    sno_cursor_t p = v.begin;
    while(p < v.end && fputc((int)*p, f) != EOF) p++;
    return p - v.begin;
}

sno_view_t sno_input(FILE* f, char* buf, size_t size) {
    if (!f || !buf || size < 2) return sno_fail();
    sno_view_t v;
    char* p = buf;
    int c = EOF;

    while ((size_t)(p - buf) < size - 1 && (c = fgetc(f)) != EOF && c != '\n')
        *p++ = (char)c;

    if (c == EOF && p == buf)   return sno_fail();                  // EOF before any chars

    if ((size_t)(p - buf) == size - 1)
        while ((c = fgetc(f)) != EOF && c != '\n');                 // truncate & discard rest of line loop

    *p = '\0';
    v.begin = buf;
    v.end   = p;
    return v;
}

sno_view_t sno_trim(sno_view_t v) {
    if(sno_failed(v)) return v;
    sno_cursor_t i = v.end - 1;
    while(i >= v.begin && sno_char_in_cstr(*i--, SNO_POSIX_WHITESPACE)) v.end--;
    return v;
}

bool sno_ident(sno_view_t u, sno_view_t v) {
    if(sno_failed(u) || sno_failed(v) || sno_size(u) != sno_size(v)) return false;
    sno_cursor_t p = u.begin, q = v.begin;
    while (p < u.end && *p == *q) p++, q++;
    return p == u.end;
}

bool sno_differ(sno_view_t u, sno_view_t v) { return !sno_ident(u, v); }
