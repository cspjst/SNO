/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.4
 *
 * @brief SNOBOL4 Program Error Messages
 *
 * A conditionally fatal error (1..16) halts unless errors are allowed: sno_err_limit(n)
 * is SNOBOL's &ERRLIMIT = n. While the limit is above zero, each such error uses one up,
 * is recorded for sno_err_type() (&ERRTYPE), and the call that raised it fails instead.
 * Unconditionally fatal errors (17..28) always halt. Halting prints the message and exits
 * with the error number, so a batch file can test ERRORLEVEL (Green Book 10.2.2).
 */
#ifndef SNO_ERROR_H
#define SNO_ERROR_H

#include "sno_types.h"

/**
 * Green Book 10.2.3 Program Error Messages
 * 1..16 conditionally fatal, 17..28 unconditionally fatal
 */
typedef enum {
    SNO_ERR_NONE                  =  0,  // NO ERROR
    SNO_ERR_ILLEGAL_DATA_TYPE     =  1,  // ILLEGAL DATA TYPE
    SNO_ERR_ARITHMETIC            =  2,  // ERROR IN ARITHMETIC OPERATION
    SNO_ERR_ARRAY_REFERENCE       =  3,  // ERRONEOUS ARRAY OR TABLE REFERENCE
    SNO_ERR_NULL_STRING           =  4,  // NULL STRING IN ILLEGAL CONTEXT
    SNO_ERR_UNDEFINED_FUNCTION    =  5,  // UNDEFINED FUNCTION OR OPERATION
    SNO_ERR_PROTOTYPE             =  6,  // ERRONEOUS PROTOTYPE
    SNO_ERR_UNKNOWN_KEYWORD       =  7,  // UNKNOWN KEYWORD
    SNO_ERR_VARIABLE_REQUIRED     =  8,  // VARIABLE NOT PRESENT WHERE REQUIRED
    SNO_ERR_ENTRY_POINT           =  9,  // ENTRY POINT OF FUNCTION NOT LABEL
    SNO_ERR_ILLEGAL_ARGUMENT      = 10,  // ILLEGAL ARGUMENT TO PRIMITIVE FUNCTION
    SNO_ERR_READING               = 11,  // READING ERROR
    SNO_ERR_ILLEGAL_IO_UNIT       = 12,  // ILLEGAL I/O UNIT
    SNO_ERR_DATA_TYPE_LIMIT       = 13,  // LIMIT ON DEFINED DATA TYPES EXCEEDED
    SNO_ERR_NEGATIVE_NUMBER       = 14,  // NEGATIVE NUMBER IN ILLEGAL CONTEXT
    SNO_ERR_STRING_OVERFLOW       = 15,  // STRING OVERFLOW
    SNO_ERR_PATTERN_OVERFLOW      = 16,  // OVERFLOW DURING PATTERN MATCHING
    SNO_ERR_SYSTEM                = 17,  // ERROR IN SNOBOL4 SYSTEM
    SNO_ERR_RETURN_ZERO_LEVEL     = 18,  // RETURN FROM ZERO LEVEL
    SNO_ERR_GOTO_FAILURE          = 19,  // FAILURE DURING GO TO EVALUATION
    SNO_ERR_INSUFFICIENT_STORAGE  = 20,  // INSUFFICIENT STORAGE TO CONTINUE
    SNO_ERR_STACK_OVERFLOW        = 21,  // STACK OVERFLOW
    SNO_ERR_STATEMENT_LIMIT       = 22,  // LIMIT ON STATEMENT EXECUTION EXCEEDED
    SNO_ERR_OBJECT_SIZE           = 23,  // OBJECT EXCEEDS SIZE LIMIT
    SNO_ERR_UNDEFINED_GOTO        = 24,  // UNDEFINED OR ERRONEOUS GOTO
    SNO_ERR_ARGUMENT_COUNT        = 25,  // INCORRECT NUMBER OF ARGUMENTS
    SNO_ERR_COMPILATION_LIMIT     = 26,  // LIMIT ON COMPILATION ERRORS EXCEEDED
    SNO_ERR_END_STATEMENT         = 27,  // ERRONEOUS END STATEMENT
    SNO_ERR_COMPILATION_STATEMENT = 28   // EXECUTION OF STATEMENT WITH A COMPILATION ERROR
} sno_error_t;

#define SNO_ERROR_COUNT             SNO_ERR_COMPILATION_STATEMENT
#define SNO_ERROR_LAST_CONDITIONAL  SNO_ERR_PATTERN_OVERFLOW

extern const char* const SNO_ERROR_MESSAGES[SNO_ERROR_COUNT + 1];

/**
 * 10.2.2 SNOBOL &ERRLIMIT
 * Allow n conditionally fatal errors before an error halts; clears sno_err_type().
 *
 * @note
 *  + The initial limit is 0, as SNOBOL: every error halts until this is called.
 *  + n <= 0 restores halting on every error.
 * @see     sno_err_count(), sno_err_type()
 */
void sno_err_limit(int n);

/**
 * 10.2.2 SNOBOL &ERRLIMIT
 * Return how many conditionally fatal errors are still allowed before an error halts.
 *
 * @note    Each error raised under the limit uses one up: after sno_err_limit(n) and k errors, n - k.
 * @see     sno_err_limit()
 */
int sno_err_count();

/**
 * 10.2.2 SNOBOL &ERRTYPE
 * Return the number of the last error raised, or SNO_ERR_NONE.
 *
 * @note
 *  + Sticky, as SNOBOL: not cleared by later calls that succeed - sno_err_limit() clears it.
 *  + Read-only, as SNOBOL's protected keyword: only sno_error() sets it.
 * @see     sno_err_limit()
 */
sno_error_t sno_err_type();

/**
 * 10.2.3 C ERROR
 * Raise error e where it happened: record it, then fail if the limit allows, else print and halt.
 *
 * @note
 *  + Returns only for a conditionally fatal error under the limit - the caller must then
 *    return its failure value: "failure occurs in the expression with the error" (10.2.2).
 *  + Halting prints e, its message, file, line and function, and exits with e as the ERRORLEVEL.
 * @see     sno_err_limit(), sno_err_type()
 */
#define sno_error(e) sno_error_at((e), __FILE__, __LINE__, __func__)

sno_view_t sno_error_at(sno_error_t e, const char* file, int line, const char* func);

#endif
