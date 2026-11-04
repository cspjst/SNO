#ifndef TEST_EXTRA3_H
#define TEST_EXTRA3_H

/*
 * Tests for sno_extra.h (v0.1), Green Book 3.4.2:
 * Chapter 3 - Primitive Functions, Predicates, and Operations.
 * Independent of the other chapters; the tests and their notes are in test_extra3.c.
 * Each test returns normally on success; a failed assert stops the run.
 */

void test_chr_replace(void);       // 3.4.2 REPLACE

// error handling
void test_fail_first_extra3(void); // argument errors before failure
void test_sno_errors_extra3(void); // precondition errors under &ERRLIMIT

// Run all of the above in this order and print "Pass extra 3"
void test_extra3(void);

#endif
