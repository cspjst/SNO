#ifndef TEST_CORE_H
#define TEST_CORE_H

/*
 * Tests for sno_core.h (v0.4), one independent file per Green Book chapter.
 * Each chapter has its own error tests and runner; test_core() just runs all three.
 */

#include "test_core1.h"                         // 0.1 - 1.8 
#include "test_core2.h"                         // 2.8 - 2.12
#include "test_core3.h"                         // 3.3.1 - 3.4.3

// Run test_core1(), test_core2() and test_core3() in order
void test_core(void);

#endif
