#include "sno_error.h"

#include <stdio.h>
#include <stdlib.h>

const char* const SNO_ERROR_MESSAGES[SNO_ERROR_COUNT + 1] = {
    "NO ERROR",                                         //  0
    "ILLEGAL DATA TYPE",                                //  1
    "ERROR IN ARITHMETIC OPERATION",                    //  2
    "ERRONEOUS ARRAY OR TABLE REFERENCE",               //  3
    "NULL STRING IN ILLEGAL CONTEXT",                   //  4
    "UNDEFINED FUNCTION OR OPERATION",                  //  5
    "ERRONEOUS PROTOTYPE",                              //  6
    "UNKNOWN KEYWORD",                                  //  7
    "VARIABLE NOT PRESENT WHERE REQUIRED",              //  8
    "ENTRY POINT OF FUNCTION NOT LABEL",                //  9
    "ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION",           // 10
    "READING ERROR",                                    // 11
    "ILLEGAL I/O UNIT",                                 // 12
    "LIMIT ON DEFINED DATA TYPES EXCEEDED",             // 13
    "NEGATIVE NUMBER IN ILLEGAL CONTEXT",               // 14
    "STRING OVERFLOW",                                  // 15
    "OVERFLOW DURING PATTERN MATCHING",                 // 16
    "ERROR IN SNOBOL4 SYSTEM",                          // 17
    "RETURN FROM ZERO LEVEL",                           // 18
    "FAILURE DURING GO TO EVALUATION",                  // 19
    "INSUFFICIENT STORAGE TO CONTINUE",                 // 20
    "STACK OVERFLOW",                                   // 21
    "LIMIT ON STATEMENT EXECUTION EXCEEDED",            // 22
    "OBJECT EXCEEDS SIZE LIMIT",                        // 23
    "UNDEFINED OR ERRONEOUS GOTO",                      // 24
    "INCORRECT NUMBER OF ARGUMENTS",                    // 25
    "LIMIT ON COMPILATION ERRORS EXCEEDED",             // 26
    "ERRONEOUS END STATEMENT",                          // 27
    "EXECUTION OF STATEMENT WITH A COMPILATION ERROR"   // 28
};

static int err_limit = 0;                               // &ERRLIMIT
static sno_error_t err_type = SNO_ERR_NONE;             // &ERRTYPE

void sno_err_limit(int n) { err_limit = n > 0 ? n : 0; err_type = SNO_ERR_NONE; }

int sno_err_count() { return err_limit; }

sno_error_t sno_err_type() { return err_type; }

sno_view_t sno_error_at(sno_error_t e, const char* file, int line, const char* func) {
    err_type = e;
    sno_view_t fail;
    fail.begin = fail.end = NULL;
    if (e <= SNO_ERROR_LAST_CONDITIONAL && err_limit > 0) {
        --err_limit;                                    // fail instead of halting
        return fail;
    }
    fprintf(stderr, "ERROR (%i) %s %s:%d: %s\n", e, SNO_ERROR_MESSAGES[e], file, line, func);
    exit(e);
}
