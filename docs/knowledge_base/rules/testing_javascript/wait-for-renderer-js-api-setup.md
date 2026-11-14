---
name: brave/wait-for-renderer-js-api-setup
description:
  A test does not use a renderer-side JavaScript API, such as
  window.braveSolana.on, just because its Mojo binding is done; it waits for the
  API itself to be ready.
rubric: testing
severity: blocking
---

**Rule.** A helper that waits for a provider's Mojo binding, such as
`WaitForSolanaProviderBinding()`, does NOT mean the JavaScript API the renderer
sets up on top of it is ready: the binding completes before the event emitter
(`window.braveSolana.on`) exists. Before a test calls such an API, it MUST wait
for the API itself, polling for it from C++ (for example
`typeof window.braveSolana?.on === 'function'`) until a deadline, as the
document's example does.

**Why.** Using the API as soon as the binding is done races with the renderer's
setup of it, so the test fails intermittently with an undefined function.

## Examples

Bad:

```cpp
WaitForSolanaProviderBinding();
// window.braveSolana.on might not be ready yet!
ASSERT_TRUE(content::ExecJs(web_contents,
                            "window.braveSolana.on('connect', onConnect)"));
```

Good:

```cpp
const base::TimeTicks deadline =
    base::TimeTicks::Now() + TestTimeouts::action_timeout();
for (;;) {
  NonBlockingDelay(TestTimeouts::tiny_timeout());
  if (content::EvalJs(web_contents,
                      "typeof window.braveSolana?.on === 'function'",
                      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                      ISOLATED_WORLD_ID_BRAVE_INTERNAL)
          .ExtractBool()) {
    break;
  }
  if (base::TimeTicks::Now() >= deadline) {
    FAIL() << "Timeout waiting for braveSolana.on";
  }
}
```
