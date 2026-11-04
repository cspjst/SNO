#ifndef TEST_EXTRA1_H
#define TEST_EXTRA1_H

/*
 * Tests for sno_extra.h (v0.1), Green Book 1.1 - 1.5:
 * Chapter 1 - Introduction to the SNOBOL Programming Language.
 * Independent of the other chapters; the tests and their notes are in test_extra1.c.
 * Each test returns normally on success; a failed assert stops the run.
 */

void test_unbind(void);            // 1.1 UNBIND
void test_output(void);            // 1.3.4 OUTPUT
void test_input(void);             // 1.3.4 INPUT
void test_str_replace(void);       // 1.5 PROXY (str_replace)

// error handling
void test_fail_first_extra1(void); // argument errors before failure
void test_sno_errors_extra1(void); // precondition errors under &ERRLIMIT

// Run all of the above in this order and print "Pass extra 1"
void test_extra1(void);

#endif
