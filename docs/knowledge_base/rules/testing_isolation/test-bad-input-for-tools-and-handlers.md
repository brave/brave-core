---
name: brave/test-bad-input-for-tools-and-handlers
description:
  A tool or handler implementation comes with unit tests for malformed, empty,
  null and out-of-range input, not only the happy path.
rubric: testing
severity: blocking
---

**Rule.** A change that adds or extends a tool or handler implementation (a
class whose job is to act on input it receives, such as an AI chat tool or a
message handler) MUST include unit tests that feed it bad input: malformed data,
empty input, null or missing values, and out-of-range parameters. Tests of the
happy path alone do not meet the rule.

**Why.** Tools and handlers receive input from outside the code that wrote them:
web content, a model, another process. The bad-input paths are the ones that
crash, misbehave or open security holes, and they are the ones a happy-path test
never exercises.

## Examples

Bad:

```cpp
TEST_F(NavigateToolTest, NavigatesToUrl) {
  RunTool(R"({"url": "https://example.com"})");
  EXPECT_EQ(GetLastNavigatedUrl(), GURL("https://example.com"));
}
```

Good:

```cpp
TEST_F(NavigateToolTest, NavigatesToUrl) {
  RunTool(R"({"url": "https://example.com"})");
  EXPECT_EQ(GetLastNavigatedUrl(), GURL("https://example.com"));
}

TEST_F(NavigateToolTest, RejectsMalformedAndMissingUrl) {
  EXPECT_THAT(RunTool(R"({"url": "not a url"})"), IsError());
  EXPECT_THAT(RunTool(R"({})"), IsError());
  EXPECT_THAT(RunTool(""), IsError());
}
```
