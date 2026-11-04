#ifndef TEST_EXTRA_H
#define TEST_EXTRA_H

/*
 * Tests for sno_extra.h (v0.1), one independent file per Green Book chapter.
 * Each chapter has its own error tests and runner; test_extra() just runs them.
 * No chapter 2 yet: sno_extra.h has nothing from Pattern Matching.
 */

#include "test_extra1.h"                        // 1.1 - 1.5
#include "test_extra3.h"                        // 3.4.2

// Run test_extra1() and test_extra3() in order
void test_extra(void);

#endif
