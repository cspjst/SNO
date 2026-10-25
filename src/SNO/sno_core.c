#include "sno_core.h"
#include "sno_types.h"
#include "sno_constants.h"
#include "sno_error.h"

#include <stdio.h>
#include <assert.h>

// 1.3.1
static const char SNO_NULL_STRING[] = "";


// C support

bool sno_char_in_set(char c, sno_view_t v) {
    if(sno_failed(v)) return false;
    sno_cursor_t p = v.begin;
    while (p < v.end && *p != c) p++;
    return p != v.end;
}

bool sno_char_in_cstr(char c, const char* cstr)  {
    if(!cstr) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    sno_cursor_t p = cstr;
    while (*p && *p != c) p++;
    return *p != '\0';
}

// Greenbook Sections:
// Chapter 1 - Introduction to the SNOBOL Programming Language

// 1.1 Assignment Statements
sno_view_t sno_bind(const char* cstr) {
    sno_view_t v;
    if (!cstr) return sno_fail();
    v.begin = v.end = cstr;
    if(cstr) while(*v.end) v.end++;
    return v;
}

sno_view_t* sno_bind_list(const char** cstrs, sno_view_t* views, size_t size) {
    if(!cstrs || !views || size < 1) { sno_error(SNO_ERR_ILLEGAL_ARGUMENT); return NULL; }
    size_t i;
    for(i = 0; i < size && cstrs[i]; ++i) {
        views[i] = sno_bind(cstrs[i]);
    }
    views[i] = sno_fail();
    return views;
}

// 1.3 Strings
sno_view_t sno_null() {
    sno_view_t v;
    v.begin = v.end = SNO_NULL_STRING;
    return v;
}

// 1.4 Pattern Matching Statements
sno_view_t sno_lit(sno_view_t* u, sno_view_t lit) {
    sno_view_t w = *u;
    sno_view_t m = sno_len(&w, sno_size(lit));
    if(!sno_ident(m, lit)) return sno_fail();
    *u = w;
    return m;
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
    if(!u) return sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if(n < 0) return sno_error(SNO_ERR_NEGATIVE_NUMBER);
    if(n > sno_size(*u) || sno_failed(*u)) return sno_fail();
    sno_view_t w;
    w.begin = u->begin;
    u->begin += n;
    w.end = u->begin;
    return w;
}

// 2.9
sno_view_t sno_break(sno_view_t* u, sno_view_t v) {
    if(!u) return sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if(sno_is_null(v)) return sno_error(SNO_ERR_NULL_STRING);
    if (sno_is_null(*u) || sno_failed(*u) || sno_failed(v)) return sno_fail();
    sno_view_t w;
    sno_cursor_t q = u->begin;
    while(q != u->end) {
        if(sno_char_in_set(*q, v)) goto match;
        q++;
    }
    return sno_fail();
match:
    w.begin = u->begin;
    u->begin = w.end = q;
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

// 2.12
sno_view_t sno_rpos(sno_view_t* u, int n) {
    if(!u) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    if(n < 0) sno_error(SNO_ERR_NEGATIVE_NUMBER);
    if(sno_size(*u) != n || sno_failed(*u)) return sno_fail();
    sno_view_t w;
    w.begin = w.end = u->begin;
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
