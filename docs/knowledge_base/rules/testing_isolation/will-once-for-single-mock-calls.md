---
name: brave/will-once-for-single-mock-calls
description:
  A mock method expected to be called once uses WillOnce, not WillRepeatedly.
rubric: testing
severity: should_fix
---

**Rule.** When the code under test should call a mocked method exactly once, its
`EXPECT_CALL` uses `.WillOnce(...)`. Keep `.WillRepeatedly(...)` for methods
that are meant to be called several times or any number of times.

**Why.** `EXPECT_CALL(...).WillRepeatedly()` accepts any number of calls, so a
bug that calls the method twice (a duplicated request, a callback run again)
still passes. `WillOnce` makes the expectation exactly one call, and the test
fails on the second.

## Examples

Bad:

```cpp
EXPECT_CALL(mock, Fetch(_)).WillRepeatedly(Return(result));
```

Good:

```cpp
EXPECT_CALL(mock, Fetch(_)).WillOnce(Return(result));
```
