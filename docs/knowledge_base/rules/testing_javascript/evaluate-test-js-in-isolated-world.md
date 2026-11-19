---
name: brave/evaluate-test-js-in-isolated-world
description:
  JavaScript a test evaluates for its own checks runs in
  ISOLATED_WORLD_ID_BRAVE_INTERNAL, so that it does not interfere with the
  page's scripts.
rubric: testing
severity: should_fix
---

**Rule.** When a test evaluates JavaScript with `content::EvalJs()` or
`content::ExecJs()`, pass `content::EXECUTE_SCRIPT_DEFAULT_OPTIONS` and
`ISOLATED_WORLD_ID_BRAVE_INTERNAL`, so that the test's code runs in Brave's
internal isolated world instead of the page's main world.

**Why.** Script evaluated in the main world shares globals, prototypes and event
handlers with the page's own scripts, so test code can change what the page
under test does, and the page can change what the test observes.

## Examples

Bad:

```cpp
content::EvalJs(web_contents, "document.getElementById('foo')");
```

Good:

```cpp
content::EvalJs(web_contents, "document.getElementById('foo')",
                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                ISOLATED_WORLD_ID_BRAVE_INTERNAL);
```
