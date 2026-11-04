# SNO C: Green Book coverage

Green Book primitive functions (table 7.3.1) and chapter 2 pattern primitives, by the
chapter and section that describe them in full. `[x]` has a SNO C equivalent in
`sno_core.h` (v0.4) or `sno_extra.h` (v0.1); `[ ]` does not yet.

## Chapter 0 - C Support (no Green Book equivalent)

- [x] 0.1 character sets - `sno_char_in_set()`, `sno_char_in_cstr()`
- [x] C string to view - `sno_bind()`, `sno_bind_list()`
- [x] view to C string - `sno_unbind()`

## Chapter 1 - Introduction to the SNOBOL4 Programming Language

- [x] 1.3.1 the null string - `sno_null()`, `sno_is_null()`
- [x] 1.3.4 `INPUT` and `OUTPUT` variables - `sno_input()`, `sno_output()`
- [x] 1.4 pattern matching statement, literal subject match - `sno_lit()`, `sno_find_lit()`
- [x] 1.5 replacement statement - `sno_str_replace()` (C proxy: in-place marker expansion)
- [x] 1.8 flow of control, `:S` and `:F` - `sno_fail()`, `sno_failed()`, `sno_succeeded()`, `S_`, `F_`

Functions introduced in 1.10 are listed under the chapter that describes them in full.

## Chapter 2 - Pattern Matching

- [x] 2.2 alternation and concatenation - by the caller: `||` and chained calls
- [x] 2.4.2 anchored mode - every SNO C match is anchored at the cursor
- [ ] 2.4.1 unanchored mode - (`sno_next_lit()` planned)
- [x] 2.5 value assignment `.` and `$` - every match returns its view
- [x] 2.8 `LEN` - `sno_len()`
- [ ] 2.9 `SPAN` - (`sno_span()` planned)
- [x] 2.9 `BREAK` - `sno_break()`
- [x] 2.10 `ANY` - `sno_any()`
- [x] 2.10 `NOTANY` - `sno_notany()`
- [x] 2.11 `TAB` - `sno_tab()`
- [x] 2.11 `RTAB` - `sno_rtab()`
- [x] 2.11 `REM` - `sno_rem()`
- [ ] 2.12 `POS` - (`sno_at()` planned?)
- [x] 2.12 `RPOS` - `sno_rpos()`
- [ ] 2.13 `FAIL` (as a pattern)
- [ ] 2.14 `FENCE`
- [ ] 2.15 `ABORT`
- [ ] 2.16 unevaluated expressions `*`
- [ ] 2.17 `ARB`
- [ ] 2.18 `ARBNO`
- [ ] 2.19 `BAL`
- [ ] 2.20 `SUCCEED`

## Chapter 3 - Primitive Functions, Predicates, and Operations

- [ ] 3.2.1 `LT`, `LE`, `EQ`, `NE`, `GE`, `GT`
- [ ] 3.2.2 `INTEGER`
- [x] 3.3.1 `IDENT` - `sno_ident()`
- [x] 3.3.1 `DIFFER` - `sno_differ()`
- [ ] 3.3.2 `LGT`
- [x] 3.4.1 `SIZE` - `sno_size()`
- [x] 3.4.2 `REPLACE` - `sno_chr_replace()`
- [x] 3.4.3 `TRIM` - `sno_trim()`
- [ ] 3.4.4 `DUPL`
- [ ] 3.4.5 `REMDR`
- [ ] 3.4.6 `DATE`
- [ ] 3.4.6 `TIME`
- [ ] 3.4.7 `EVAL`
- [ ] 3.4.8 `APPLY`
- [ ] 3.5 negation `¬` and interrogation `?`
- [ ] 3.6.1 `LOAD`
- [ ] 3.6.2 `UNLOAD`
- [ ] 3.7 `OPSYN`

## Chapter 4 - Programmer-Defined Functions

- [ ] 4.2 `DEFINE`
- [ ] 4.3 `RETURN`, `FRETURN`, `NRETURN`

## Chapter 5 - Arrays, Tables, and Defined Data Types

- [ ] 5.1 `ARRAY`
- [ ] 5.2 `TABLE`
- [ ] 5.3.1 `COPY`
- [ ] 5.3.2 `PROTOTYPE`
- [ ] 5.3.3 `ITEM`
- [ ] 5.4 `DATA`
- [ ] 5.4.1 `VALUE`

## Chapter 6 - Keywords, Names, and Code

- [ ] 6.3.1 `CODE`

## Chapter 7 - Types of Data

- [ ] 7.1 `DATATYPE`
- [ ] 7.2.1 `CONVERT`

## Chapter 8 - Tracing

- [ ] 8.1 `TRACE`
- [ ] 8.1.5 `STOPTR`
- [ ] 8.2.2 `ARG`
- [ ] 8.2.2 `LOCAL`
- [ ] 8.2.2 `FIELD`
- [ ] 8.4 `DUMP`

## Chapter 9 - Input and Output

- [ ] 9.5 `OUTPUT()` association
- [ ] 9.6 `INPUT()` association
- [ ] 9.7.1 `BACKSPACE`
- [ ] 9.7.2 `DETACH`
- [ ] 9.7.3 `ENDFILE`
- [ ] 9.7.4 `REWIND`

## Chapter 10 - Running a SNOBOL4 Program

- [x] 10.2.2 `&ERRLIMIT`, `&ERRTYPE` - `sno_err_limit()`, `sno_err_type()`, `sno_err_count()`

## Chapter 11 - Programming Details and Storage Management

- [ ] 11.6.1 `COLLECT`
- [ ] 11.6.2 `CLEAR`
