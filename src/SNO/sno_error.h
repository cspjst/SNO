/**
 * @author Jeremy Simon Thornton
 * @copyright Copyright (c) 2026 Jeremy Simon Thornton
 * @license MIT License — see LICENSE file or https://opensource.org/licenses/MIT
 * @version 0.3
 *
 * @brief SNOBOL4 Program Error Messages
 */
#ifndef SNO_ERROR_H
#define SNO_ERROR_H

#include <stdlib.h>

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

// Source - https://stackoverflow.com/a/67421741
// Posted by mediocrevegetable1, modified by community. See post 'Timeline' for change history
// Retrieved 2026-10-21, License - CC BY-SA 4.0

#define sno_error(e)                                   \
        (void) (fprintf(stderr, "ERROR (%i) %s %s:%d: %s\n",   \
        e, SNO_ERROR_MESSAGES[e], __FILE__, __LINE__, __FUNCTION__), exit(e))



#endif
