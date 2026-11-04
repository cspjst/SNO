#ifndef TEST_CORE1_H
#define TEST_CORE1_H

/*
 * Tests for sno_core.h (v0.4), Green Book 0.1 - 1.11:
 * Chapter 0 - C Support and Chapter 1 - Introduction to the SNOBOL Programming Language.
 * Independent of the other chapters; the tests and their notes are in test_core1.c.
 * Each test returns normally on success; a failed assert stops the run.
 */

void test_char_in(void);     // 0.1 character sets
void test_bind(void);        // 1.1 BIND
void test_bind_list(void);   // 1.1 BIND LIST
void test_fail_null(void);   // 1.3.1 NULL / 1.8 FAIL
void test_lit(void);         // 1.4 LIT
void test_find_lit(void);    // 1.4 FIND LIT
void test_succeeded(void);   // 1.8 SUCCEEDED
void test_size(void);        // 1.10.1 SIZE
void test_trim(void);        // 1.11 TRIM

// error handling
void test_fail_first1(void); // argument errors before failure
void test_sno_errors1(void); // precondition errors under &ERRLIMIT

// Run all of the above in this order and print "Pass core 1"
void test_core1(void);

#endif
