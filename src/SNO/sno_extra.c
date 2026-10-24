#include "sno_extra.h"
#include "sno_error.h"
#include "sno_core.h"

#include <stdlib.h>
#include <string.h>

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

// 1.5
sno_view_t sno_proxy(char* buf, size_t size, const char** from, const char** to) {
    if(!buf || !size || !from || !to) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    size_t i;
    for(i = 0; from[i] != NULL; ++i) if(to[i] == NULL) return sno_fail();
    if(from[i] || to[i] || !from[0] || !to[0]) return sno_fail();

    char* temp = (char*)calloc(size, 1);
    char *q = temp;
    size_t temp_size = 0;
    const char* p = buf;

    while(*p) {
        for(size_t j = 0; j < i; ++j) {
            if(strncmp(p, from[j], strlen(from[j])) == 0) {
                temp_size += strlen(to[j]);
                if(temp_size >= size) goto error;
                strcpy(q, to[j]);
                p += strlen(from[j]);
                q += strlen(to[j]);
                break;
            }
        }
        *q++ = *p++;
        if(++temp_size >= size) goto error;
    }
    strcpy(buf, temp);
    free(temp);
    return sno_bind(buf);
error:
    free(temp);
    sno_error(SNO_ERR_STRING_OVERFLOW);
}

// 3.4.2
sno_view_t sno_replace(char* buf, size_t size, sno_view_t from, sno_view_t to) {
    return sno_fail();
}
