#include "sno_core.h"

sno_view_t bind(const char* cstr) {
    sno_view_t v;
    v.begin = v.end = cstr;
    if (cstr) while (*v.end) v.end++;
    return v;
}
