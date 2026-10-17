#include "sno_core.h"
#include <stdio.h>

// 1.3.1
static const char SNO_NULL_STRING[] = "";

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

sno_view_t sno_bind(const char* cstr) {
    sno_view_t v;
    v.begin = v.end = cstr;
    if(cstr) while(*v.end) v.end++;
    return v;
}

char* sno_unbind(sno_view_t v, char* buf, size_t size) {
    if(!buf || !size) return NULL;
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

int sno_output(FILE* f, sno_view_t v) {
    if(!f) return 0;
    sno_cursor_t p = v.begin;
    while(p < v.end && fputc((int)*p, f) != EOF) p++;
    return p - v.begin;
}

sno_view_t sno_input(FILE* f, char* buf, size_t size) {
    if(!f || !buf || !size) return sno_null();
    sno_view_t v;
    v.begin = buf;
    v.end = buf + size;
    while (v.begin < v.end && (*buf++ = fgetc(f)) != EOF) v.begin++;
    return v;
}
