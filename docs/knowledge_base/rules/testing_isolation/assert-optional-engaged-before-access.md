---
name: brave/assert-optional-engaged-before-access
description:
  Tests check an optional with ASSERT_NE(opt, std::nullopt) or
  ASSERT_OK_AND_ASSIGN before reading its value, and never fall back with
  value_or().
rubric: testing
severity: should_fix
---

**Rule.** Before a test reads the value of an optional, it asserts fatally that
the optional holds one: `ASSERT_NE(optional, std::nullopt)`, or
`ASSERT_OK_AND_ASSIGN` to check and unwrap an optional a call returns. A test
MUST NOT substitute a fallback with `value_or()` (`value_or("")`, `value_or(0)`)
for a value it expects to be there.

**Why.** A fallback turns "the value is missing" into an ordinary value, so the
test goes on comparing against the fallback and either passes by accident or
fails somewhere unrelated. A fatal assertion stops the test at the real fault
and prints the empty optional.

## Examples

Bad:

```cpp
EXPECT_EQ(map[entry->uuid.value_or("")], expected_content);
```

Good:

```cpp
ASSERT_NE(entry->uuid, std::nullopt);
EXPECT_EQ(map[*entry->uuid], expected_content);
```
