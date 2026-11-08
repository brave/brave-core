---
name: brave/test-both-sides-of-guard-conditions
description:
  Code behind a guard condition is tested both while the guard blocks it and
  after the guard releases it.
rubric: testing
severity: should_fix
---

**Rule.** When the code under test is gated by a condition (blocked while
loading, disabled until initialised, refused until a permission is granted), the
tests cover both sides: that the behaviour is blocked while the guard is active,
AND that it proceeds correctly once the guard is released. Flag tests of guarded
code that only set up the released state.

**Why.** A test of the happy path alone passes when the guard is missing
altogether, and a test of the blocked state alone passes when the guard never
releases. Only both together pin down the guard.

## Examples

Bad:

```cpp
TEST_F(MyTest, ActionSucceeds) {
  SetupCompleteState();
  EXPECT_TRUE(DoAction());
}
```

Good:

```cpp
TEST_F(MyTest, ActionBlockedDuringLoad) {
  EXPECT_FALSE(DoAction());
}

TEST_F(MyTest, ActionSucceedsAfterLoad) {
  CompleteLoading();
  EXPECT_TRUE(DoAction());
}
```
