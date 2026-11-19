---
name: brave/test-public-api-not-private-state
description:
  Tests assert what the public interface of the class under test returns or
  does, not the values of its private members.
rubric: testing
severity: should_fix
---

**Rule.** Check the behaviour of the class under test through its public API
(`object->IsReady()`, `object->GetValue()`). Tests MUST NOT assert on private
data members (`object->private_state_`), whether reached through a friend
declaration, a `#define private public` or an accessor that only exposes
internal state.

**Why.** A test bound to internals breaks whenever the implementation changes,
even when the behaviour is the same, so it resists refactoring instead of
protecting it. A test written against the public interface also shows other
developers how the class is meant to be used.

## Examples

Bad:

```cpp
EXPECT_EQ(object->private_state_, 42);
```

Good:

```cpp
EXPECT_TRUE(object->IsReady());
EXPECT_EQ(object->GetValue(), 42);
```
