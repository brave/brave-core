---
name: brave/base-time-over-c-time
description:
  'C++ code must use base::Time, base::TimeDelta and related classes, never
  std::time, ctime functions or time_t.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER use `std::time()`, `time_t`, `struct tm`/`std::tm` or the
`<ctime>` functions (`localtime`, `gmtime`, `mktime`, `difftime`, `strftime`).
Use `base::Time`, `base::TimeDelta`, `base::TimeTicks` and their helpers
(`base::Time::Now()`, `base::Time::Exploded`, `base::TimeFormat*()`) instead.

**Why.** The C time API is not type-safe (a `time_t` is a bare integer of
unspecified unit), is not thread-safe in its `localtime`/`gmtime` forms, and
behaves differently across platforms. The base classes are cross-platform, carry
their units in their types, and can be mocked in tests with
`base::test::TaskEnvironment`.

## Examples

Bad:

```cpp
time_t now = std::time(nullptr);
if (now - last_check_ > 24 * 60 * 60) {
  Check();
}
```

Good:

```cpp
base::Time now = base::Time::Now();
if (now - last_check_ > base::Days(1)) {
  Check();
}
```
