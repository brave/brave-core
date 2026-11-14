---
name: brave/check-eval-js-results
description:
  Every content::EvalJs() result in a test is checked with EXPECT_*/ASSERT_*,
  never discarded.
rubric: testing
severity: blocking
---

**Rule.** Compare the result of every `content::EvalJs()` call with
`EXPECT_EQ`/`EXPECT_TRUE` (or their `ASSERT_*` forms). NEVER discard it: an
`std::ignore =` or `(void)` in front of `EvalJs()`, a stored result that no
assertion reads, or an `.ExtractBool()` whose value is dropped all count as
unchecked. When only the script's side effect matters, check it with
`EXPECT_TRUE(content::ExecJs(...))`.

**Why.** `EvalJs()` reports a script exception in its result instead of failing
the test, so a call nobody checks swallows the error: a misspelt or nonsensical
expression appears to pass, and the test verifies nothing.

## Examples

Bad:

```cpp
std::ignore = content::EvalJs(web_contents, "window.solana.isConnected");
```

Good:

```cpp
EXPECT_EQ(true, content::EvalJs(web_contents, "window.solana.isConnected"));
```
