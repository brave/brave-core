---
name: brave/verify-order-of-ordered-results
description:
  A test of a function that returns results in a defined order asserts that
  order, with testing::ElementsAre, not only membership.
rubric: testing
severity: should_fix
---

**Rule.** When the function under test promises an order (most recent first,
sorted by score), the test asserts the whole sequence in that order, preferably
with `EXPECT_THAT(results, testing::ElementsAre(...))`. Checking the size and
that each item is present (`Contains`, `UnorderedElementsAre`, a `ContainsUrl()`
helper) does not meet the rule.

**Why.** A membership check passes when the order is wrong, so the ordering the
function promises goes untested and can regress unnoticed. `ElementsAre` checks
the size and the order at once and prints the full sequence when it fails.

## Examples

Bad:

```cpp
EXPECT_EQ(results.size(), 3u);
EXPECT_TRUE(ContainsUrl(results, "https://a.com"));
EXPECT_TRUE(ContainsUrl(results, "https://b.com"));
```

Good:

```cpp
EXPECT_THAT(results,
            testing::ElementsAre(
                testing::Field(&Result::url, "https://c.com"),  // most recent
                testing::Field(&Result::url, "https://b.com"),
                testing::Field(&Result::url, "https://a.com")));  // oldest
```
