---
name: brave/no-check-for-test-expectations
description:
  'Test code reports a failed expectation with ADD_FAILURE(), EXPECT_* or
  ASSERT_*, never by CHECK-crashing the test binary.'
rubric: testing
severity: should_fix
---

**Rule.** In test code, NEVER use `CHECK()` (or `CHECK_EQ()` and the like) to
verify what the test expects. Use `EXPECT_*`/`ASSERT_*`, or `ADD_FAILURE()`
where no assertion macro fits, such as in a helper or a callback.

**Why.** A `CHECK()` crashes the whole test binary: the failure loses its gtest
message, and the remaining tests in the shard do not run.

## Examples

Bad:

```cpp
service->Load(base::BindLambdaForTesting([&](bool success) {
  CHECK(success);
}));
```

Good:

```cpp
service->Load(base::BindLambdaForTesting([&](bool success) {
  if (!success) {
    ADD_FAILURE() << "Load failed";
  }
}));
```
