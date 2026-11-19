---
name: brave/death-test-suite-suffix
description:
  'A test suite containing death tests is named with the DeathTest suffix.'
rubric: testing
severity: blocking
---

**Rule.** The suite of every test using `EXPECT_DEATH`, `ASSERT_DEATH`,
`EXPECT_CHECK_DEATH` or another death-test macro MUST be named with the
`DeathTest` suffix: give the fixture a `*DeathTest` alias when the same fixture
also runs ordinary tests.

**Why.** GoogleTest runs suites named `*DeathTest` before the others, so the
process forks for a death test before other tests start threads. Without the
suffix, death tests become unreliable.

## Examples

Bad:

```cpp
using MyFeatureTest = testing::Test;
TEST_F(MyFeatureTest, CrashesOnNull) {
  EXPECT_DEATH(Process(nullptr), "");
}
```

Good:

```cpp
using MyFeatureDeathTest = testing::Test;
TEST_F(MyFeatureDeathTest, CrashesOnNull) {
  EXPECT_DEATH(Process(nullptr), "");
}
```
