---
name: brave/use-run-until-for-cxx-conditions
description:
  A test waiting for C++ state or an object property to change polls it with
  base::test::RunUntil(), not with a hand-written loop that spins run loops,
  sleeps or delays.
rubric: testing
severity: should_fix
---

**Rule.** To wait until C++ state or an object's property reaches the value a
test needs, use `base::test::RunUntil()` with a predicate that checks it, and
assert its result as the document's examples do
(`ASSERT_TRUE(base::test::RunUntil(...))`). Do not hand-write the polling: a
`for`/`while` loop that spins `base::RunLoop`, sleeps, or calls a delay between
checks of a C++ condition is replaced by `RunUntil()`.

**Why.** `RunUntil()` runs the task queues only until the predicate holds and
fails the test with a clear timeout otherwise. A hand-written loop either busy
waits, relies on arbitrary delays or on `RunUntilIdle()`, or never ends when the
condition does not come true.

## Examples

Bad:

```cpp
while (!tab_helper()->speedreader_bubble_view()) {
  base::RunLoop().RunUntilIdle();
}
```

Good:

```cpp
ASSERT_TRUE(base::test::RunUntil([this]() {
  return tab_helper()->speedreader_bubble_view() != nullptr;
}));
```
