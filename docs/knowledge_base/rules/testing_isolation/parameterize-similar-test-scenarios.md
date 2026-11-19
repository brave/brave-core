---
name: brave/parameterize-similar-test-scenarios
description:
  Scenarios that differ only in inputs and expected outputs are one table-driven
  or parameterized test, not one copied test function each.
rubric: readability
severity: nit
---

**Rule.** When a change adds several tests whose bodies are the same apart from
their inputs and expected outputs, write them as one test over a table of cases
(each case traced with `SCOPED_TRACE(tc.name)`), or as a value-parameterized
suite with `INSTANTIATE_TEST_SUITE_P` for larger suites. Flag a run of
near-identical `TEST`/`TEST_F` functions.

**Why.** Copied test bodies drift apart as they are edited, hide which detail
actually differs between cases, and make adding a case a copy-and-paste job. A
table makes the cases and their differences visible in one place.

## Examples

Bad:

```cpp
TEST_F(MyTest, HandleTypeA) { EXPECT_EQ(Handle(ActionType::kA), "expected_a"); }
TEST_F(MyTest, HandleTypeB) { EXPECT_EQ(Handle(ActionType::kB), "expected_b"); }
TEST_F(MyTest, HandleTypeC) { EXPECT_EQ(Handle(ActionType::kC), "expected_c"); }
```

Good:

```cpp
TEST_F(MyTest, HandleAllTypes) {
  const TestCase cases[] = {
      {"TypeA", ActionType::kA, "expected_a"},
      {"TypeB", ActionType::kB, "expected_b"},
      {"TypeC", ActionType::kC, "expected_c"},
  };
  for (const auto& tc : cases) {
    SCOPED_TRACE(tc.name);
    EXPECT_EQ(Handle(tc.type), tc.expected);
  }
}
```
