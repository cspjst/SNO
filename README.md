# SNO
C function implementation of the SNOBOL4 pattern matching semantics  as defined in 'The SNOBOL4 Programming Language' 2nd edition,   Griswold, Poage &amp; Polonsky, 1971 (aka "The Green Book").

## Design Decisions:
+ Immutable Subject - null treminated C string.
+ No Backtracking - only SNOBOL 'anchored' mode, simplicity and efficiency.
+ Context-free recognition remains possible through explicit recursion — not hidden engine magic.
+ String View - [begin, end) half-open span of last match — zero-copy substring.
+ Failure Contract - every pattern function on failure leaves the string view unchanged.

## SNOBOL Translation Principles:
+ All operations mutate subject->begin on SUCCESS
+ Cursor remains UNCHANGED on FAILURE (atomic rollback)
+ Return true/false enables natural &&/|| chaining
+ Only NULL arguments cause true errors (not "no match")
