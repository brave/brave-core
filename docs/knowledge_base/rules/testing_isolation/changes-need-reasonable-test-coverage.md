---
name: brave/changes-need-reasonable-test-coverage
description:
  New logic, branches and edge cases come with tests, except trivial UI and code
  following an untested local pattern; hard-to-test code is refactored to be
  testable.
rubric: testing
severity: should_fix
---

**Rule.** When a change adds logic, branches or edge cases that can be tested,
it adds tests that exercise them. Use judgment and flag obvious gaps, not every
uncovered line: the aim is Chromium's target of at least 80% coverage, ideally
90%, weighed on the lines the change touches, and not the
`NOTREACHED()`/`CHECK()`/`LOG(FATAL)` paths past ~90%. NEVER ask for a dedicated
test of trivial UI with no logic (a toggle wired to a pref), or of code that
follows an existing untested pattern around it. When code is hard to test,
suggest the refactor that makes it testable (extract a pure function, inject the
dependency, separate I/O from computation) rather than accepting it untested.

**Why.** Untested logic regresses silently, and "tests later" rarely happens.
Requiring tests for code with nothing to test, or for one more instance of an
established untested pattern, only adds noise to reviews.

## Examples

Bad:

```cpp
// New parsing logic, with no test in the change.
std::optional<Rule> ParseRule(std::string_view line) {
  if (line.starts_with("!")) {
    return std::nullopt;
  }
  ...
}
```

Good:

```cpp
TEST(ParseRuleTest, SkipsCommentLines) {
  EXPECT_EQ(ParseRule("! comment"), std::nullopt);
}
```
