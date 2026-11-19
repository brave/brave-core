---
name: brave/no-hardcoded-js-timeouts-in-tests
description:
  JavaScript that a test evaluates does not wait with a hardcoded setTimeout()
  delay; the test waits for the actual condition, for example with
  base::test::RunUntil() in C++.
rubric: testing
severity: blocking
---

**Rule.** NEVER make a test wait by evaluating JavaScript with a hardcoded
delay, such as `setTimeout(() => { ... }, 1200)` passed to `content::EvalJs()`
or `content::ExecJs()`. Wait for the actual condition the test needs instead,
for example by checking the C++ state with `base::test::RunUntil()`.

**Why.** A fixed delay guesses how long the awaited work takes. It is
unreliable, failing on slow or loaded bots and sanitizer builds, and it slows
the test down everywhere else.

## Examples

Bad:

```cpp
content::EvalJs(web_contents, R"(
  setTimeout(() => { /* wait for something */ }, 1200);
)");
```

Good:

```cpp
ASSERT_TRUE(base::test::RunUntil([&]() {
  return template_url_service->GetTemplateURLForHost(host) != nullptr;
}));
```
