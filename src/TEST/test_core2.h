#ifndef TEST_CORE2_H
#define TEST_CORE2_H

/*
 * Tests for sno_core.h (v0.4), Green Book 2.8 - 2.12:
 * Chapter 2 - Pattern Matching.
 * Independent of the other chapters; the tests and their notes are in test_core2.c.
 * Each test returns normally on success; a failed assert stops the run.
 */

void test_len(void);         // 2.8 LEN
void test_break(void);       // 2.9 BREAK
void test_any_notany(void);  // 2.10 ANY / NOTANY
void test_rtab(void);        // 2.11 RTAB
void test_rem(void);         // 2.11 REM
void test_tab(void);         // 2.11 TAB
void test_rpos(void);        // 2.12 RPOS

// error handling
void test_fail_first2(void); // argument errors before failure
void test_sno_errors2(void); // precondition errors under &ERRLIMIT

// Run all of the above in this order and print "Pass core 2"
void test_core2(void);

#endif
