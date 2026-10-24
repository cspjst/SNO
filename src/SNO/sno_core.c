#include "sno_core.h"
#include "sno_types.h"
#include "sno_constants.h"
#include "sno_error.h"

#include <stdio.h>
#include <assert.h>

// 1.3.1
static const char SNO_NULL_STRING[] = "";

bool sno_char_in_set(char c, sno_view_t v) {
    if(sno_failed(v)) return false;
    sno_cursor_t p = v.begin;
    while (p < v.end && *p != c) p++;
    return p != v.end;
}

// C support

bool sno_char_in_cstr(char c, const char* cstr)  {
    if(!cstr) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
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

// Greenbook Sections:
// Chapter 1 - Introduction to the SNOBOL Programming Language

// 1.1 Assignment Statements

char* sno_unbind(sno_view_t v, char* buf, size_t size) {
    if(!buf) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if(sno_failed(v) || !size) return NULL;
    char* p = buf;
    char* buf_end = p + size - 1;
    while(v.begin < v.end && p < buf_end) *p++ = *v.begin++;
    *p = '\0';
    return buf;
}

// 1.3 Strings

sno_view_t sno_null() {
    sno_view_t v;
    v.begin = v.end = SNO_NULL_STRING;
    return v;
}

// 1.3.4 IO
int sno_output(FILE* f, sno_view_t v) {
    if(!f) sno_error(SNO_ERR_ILLEGAL_IO_UNIT);
    if(sno_failed(v)) return 0;
    sno_cursor_t p = v.begin;
    while(p < v.end && fputc((int)*p, f) != EOF) p++;
    return p - v.begin;
}

sno_view_t sno_input(FILE* f, char* buf, size_t size) {
    if(!f) sno_error(SNO_ERR_ILLEGAL_IO_UNIT);
    if(!buf || size < 2) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    sno_view_t v;
    char* p = buf;
    int c = EOF;

    while ((size_t)(p - buf) < size - 1 && (c = fgetc(f)) != EOF && c != '\n')
        *p++ = (char)c;

    if(c == EOF && ferror(f)) sno_error(SNO_ERR_READING);       // read error, not EOF
    if (c == EOF && p == buf) return sno_fail();                // EOF before any chars
    if ((size_t)(p - buf) == size - 1) {
        while ((c = fgetc(f)) != EOF && c != '\n');             // truncate & discard rest of line
        if(c == EOF || ferror(f)) sno_error(SNO_ERR_READING);
    }

    *p = '\0';
    v.begin = buf;
    v.end   = p;
    return v;
}

// 1.8 Flow Control

sno_view_t sno_fail() {
    sno_view_t v;
    v.begin = v.end = NULL;
    return v;
}

// 1.10 Functions

// 1.11
sno_view_t sno_trim(sno_view_t v) {
    if(sno_failed(v)) return v;
    sno_cursor_t i = v.end - 1;
    while(i >= v.begin && sno_char_in_cstr(*i--, SNO_POSIX_WHITESPACE)) v.end--;
    return v;
}

// 2.0 Pattern Matching

// 2.8
sno_view_t sno_len(sno_view_t* u, int n) {
    if(!u) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if(n < 0) sno_error(SNO_ERR_NEGATIVE_NUMBER);
    if(n > sno_size(*u) || sno_failed(*u)) return sno_fail();
    sno_view_t w;
    w.begin = u->begin;
    u->begin += n;
    w.end = u->begin;
    return w;
}

// 2.10
sno_view_t sno_any(sno_view_t* u, sno_view_t v) {
    if(!u) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if (sno_failed(*u) || sno_failed(v)) return sno_fail();
    if(sno_is_null(v)) sno_error(SNO_ERR_NULL_STRING);
    if (sno_is_null(*u)) return sno_fail();
    if (!sno_char_in_set(*u->begin, v)) return sno_fail();
    sno_view_t w;
    w.begin = u->begin;
    u->begin++;
    w.end = u->begin;
    return w;
}

// 2.10
sno_view_t sno_notany(sno_view_t* u, sno_view_t v) {
    if(!u) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if (sno_failed(*u) || sno_failed(v)) return sno_fail();
    if(sno_is_null(v)) sno_error(SNO_ERR_NULL_STRING);
    if (sno_is_null(*u)) return sno_fail();
    if (sno_char_in_set(*u->begin, v)) return sno_fail();
    sno_view_t w;
    w.begin = u->begin;
    u->begin++;
    w.end = u->begin;
    return w;
}

// 2.11
sno_view_t sno_rtab(sno_view_t* u, int n) {
    if(!u) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if(n < 0) sno_error(SNO_ERR_NEGATIVE_NUMBER);
    if(u->end - n < u->begin || sno_failed(*u)) return sno_fail();
    sno_view_t w;
    w.begin = u->begin;
    u->begin = u->end - n;
    w.end = u->begin;
    return w;
}

// 2.11
sno_view_t sno_tab(sno_view_t* u, int n) {
    if(!u) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if(n < 0) sno_error(SNO_ERR_NEGATIVE_NUMBER);
    if(u->end - n < u->begin || sno_failed(*u)) return sno_fail();
    sno_view_t w;
    w.begin = u->begin;
    u->begin += n;
    w.end = u->begin;
    return w;
}

// 3.0 Primitive Functions, Predicates, and Operations

// 3.3.1
bool sno_ident(sno_view_t u, sno_view_t v) {
    if(sno_failed(u) || sno_failed(v) || sno_size(u) != sno_size(v)) return false;
    sno_cursor_t p = u.begin, q = v.begin;
    while (p < u.end && *p == *q) p++, q++;
    return p == u.end;
}

// 3.3.1
bool sno_differ(sno_view_t u, sno_view_t v) {
    if(sno_failed(u) || sno_failed(v)) return false;    // sno_differ has to check for failure itself before negating anything
    if(sno_size(u) != sno_size(v)) return true;
    sno_cursor_t p = u.begin, q = v.begin;
    while (p < u.end && *p == *q) p++, q++;
    return p != u.end;
}
