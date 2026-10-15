#include "sno_core.h"
#include <stdio.h>

bool sno_char_in_set(char c, sno_view_t v) {
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

sno_view_t bind(const char* cstr) {
    sno_view_t v;
    v.begin = v.end = cstr;
    if(cstr) while(*v.end) v.end++;
    return v;
}

char* sno_unbind(sno_view_t v, char* buf) {
    if(!buf) return NULL;
    char* p = buf;
    while(v.begin < v.end) *p++ = *v.begin++;
    *p = '\0';
    return buf;
}

sno_view_t sno_null() {
    sno_view_t v;
    v.begin = v.end = 0;
    return v;
}

int sno_output(sno_view_t v, FILE* f) {
    if(!f) return 0;
    sno_cursor_t p = v.begin;
    while(p < v.end && fputc((int)*p, f) != EOF) p++;
    return p - v.begin;
}
