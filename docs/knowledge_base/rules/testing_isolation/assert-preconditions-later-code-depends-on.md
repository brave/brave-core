---
name: brave/assert-preconditions-later-code-depends-on
description:
  A test check that later statements rely on (non-null, has a value, has enough
  elements) is a fatal ASSERT_*, not an EXPECT_*.
rubric: testing
severity: should_fix
---

**Rule.** When the statements after a check depend on it holding (dereferencing
a pointer, reading an optional, indexing a container, running a callback), the
check is an `ASSERT_*`. Flag an `EXPECT_*` that guards such a use, as in
`EXPECT_TRUE(item != nullptr);` followed by `item->DoSomething();`.

**Why.** A failed `EXPECT_*` lets the test carry on, so the next line
dereferences null and crashes the test binary, or produces a cascade of
secondary failures that hide the real one. `ASSERT_*` stops the test at the
first failure.

## Examples

Bad:

```cpp
EXPECT_TRUE(item != nullptr);
item->DoSomething();
```

Good:

```cpp
ASSERT_TRUE(item != nullptr);
item->DoSomething();
```
