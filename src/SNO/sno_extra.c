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
sno_view_t sno_str_replace(char* buf, size_t size, const char** from, const char** to) {
    if(!buf || !memchr(buf, '\0', size) || !from || !to) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    size_t i, j, k;
    for(i = 0; from[i] != NULL; ++i) if(to[i] == NULL || !*from[i]) return sno_fail();
    if(to[i] || i == 0) return sno_fail();

    char* temp = (char*)calloc(size, 1);
    if(!temp) sno_error(SNO_ERR_STRING_OVERFLOW);
    char* q = temp;
    const char* p = buf;

    while(*p) {
        for(j = 0; j < i && strncmp(p, from[j], strlen(from[j])) != 0; ++j);
        k = j < i ? strlen(to[j]) : 1;      // copy a replacement or one char
        if(q + k >= temp + size) goto error;
        memcpy(q, j < i ? to[j] : p, k);
        q += k;
        p += j < i ? strlen(from[j]) : 1;
    }
    strcpy(buf, temp);
    free(temp);
    return sno_bind(buf);
error:
    free(temp);
    sno_error(SNO_ERR_STRING_OVERFLOW);
}

// 3.4.2
// Green Book, 3.4.2, page 81, word for word:
// In the case of the multiple occurrence of a character in the second argument, the rightmost correspondence holds.
// Thus, following execution of the statement TEXT = REPLACE('FEET', 'EE', 'AO') the variable TEXT has value FOOT.
sno_view_t sno_chr_replace(char* buf, size_t size, const char* from, const char* to) {
    if(!buf || !memchr(buf, '\0', size) || !from || !to) sno_error(SNO_ERR_ILLEGAL_ARGUMENT);
    size_t i, n = strlen(from);
    if(n == 0 || n != strlen(to)) return sno_fail();

    // rightmost correspondence likely because a table used
    unsigned char table[256];
    for(i = 0; i < 256; ++i) table[i] = (unsigned char)i;
    for(i = 0; i < n; ++i) table[(unsigned char)from[i]] = (unsigned char)to[i];

    __asm {
        .8086
        push ds
        push es

        lds     si, buf
        les     di, buf
        lea     bx, table
        cld

NEXT:   lodsb
        test    al, al
        jz      DONE
        xlat    // 8086’s version of the IBM 360’s TR instruction, and SNOBOL4 grew up on the 360
        stosb
        jmp     NEXT

DONE:   pop     es
        pop     ds
    }
    return sno_bind(buf);
}
