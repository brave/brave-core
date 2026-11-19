---
name: brave/plain-test-for-empty-fixtures
description:
  Tests use TEST() rather than TEST_F() with a fixture that adds nothing to
  testing::Test.
rubric: readability
severity: nit
---

**Rule.** When a fixture class derives from `testing::Test` and adds no members,
no `SetUp()` and no `TearDown()`, drop the class and write its tests with
`TEST()`. Keep `TEST_F()` for fixtures that provide shared state or
configuration.

**Why.** An empty fixture is boilerplate that suggests the tests share setup
they do not have, and sends the reader looking for it.

## Examples

Bad:

```cpp
class MyUtilsTest : public testing::Test {};

TEST_F(MyUtilsTest, ParsesValidInput) {
  EXPECT_EQ(Parse("abc"), "abc");
}
```

Good:

```cpp
TEST(MyUtilsTest, ParsesValidInput) {
  EXPECT_EQ(Parse("abc"), "abc");
}
```
