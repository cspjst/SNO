#ifndef TEST_CORE3_H
#define TEST_CORE3_H

/*
 * Tests for sno_core.h (v0.4), Green Book 3.3.1:
 * Chapter 3 - Primitive Functions, Predicates, and Operations.
 * Independent of the other chapters; the tests and their notes are in test_core3.c.
 * Each test returns normally on success; a failed assert stops the run.
 */

void test_ident_differ(void); // 3.3.1 IDENT / DIFFER

// Run all of the above in this order and print "Pass core 3"
void test_core3(void);

#endif
