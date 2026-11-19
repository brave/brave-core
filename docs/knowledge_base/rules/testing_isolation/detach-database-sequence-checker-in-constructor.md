---
name: brave/detach-database-sequence-checker-in-constructor
description:
  A database class with a SEQUENCE_CHECKER calls
  DETACH_FROM_SEQUENCE(sequence_checker_) in its constructor.
rubric: testing
severity: should_fix
---

**Rule.** The constructor of a database class that declares a
`SEQUENCE_CHECKER(sequence_checker_)` calls
`DETACH_FROM_SEQUENCE(sequence_checker_)`, so the checker binds to the sequence
that first uses the object rather than the one that constructed it.

**Why.** Database objects are usually constructed on one sequence and used on a
database sequence. Without the detach, the checker binds at construction, and
tests that build the object on the main thread and use it from a task runner (as
mock-based tests do) fail the sequence checks.

## Examples

Bad:

```cpp
MyDatabase::MyDatabase() = default;
```

Good:

```cpp
MyDatabase::MyDatabase() {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}
```
