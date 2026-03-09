# JavaScript Evaluation in Tests

<a id="TJ-001"></a>

## ✅ Prefer Event-Driven JavaScript Over C++ Polling

**When waiting for DOM changes, prefer JavaScript event-driven patterns (like MutationObserver) over C++ polling loops.**

Event-driven patterns are:
- More deterministic (respond immediately when the event occurs)
- More efficient (no wasted CPU cycles polling)
- Consistent with Chromium patterns (see `service_worker_internals_ui_browsertest.cc`)

**BEST - MutationObserver for DOM changes:**
```cpp
// Pattern from Chromium's service_worker_internals_ui_browsertest.cc
static constexpr char kWaitForTextScript[] = R"(
  (function() {
    const element = document.getElementById($1);
    const expected = $2;

    function getText() {
      return element.tagName === 'INPUT' || element.tagName === 'TEXTAREA'
          ? element.value : element.innerText;
    }

    if (getText() === expected) {
      return getText();
    }

    return new Promise(function(resolve) {
      const observer = new MutationObserver(function() {
        if (getText() === expected) {
          observer.disconnect();
          resolve(getText());
        }
      });
      observer.observe(element,
          {childList: true, subtree: true, characterData: true});
    });
  })()
)";
std::string updated_text =
    content::EvalJs(web_contents,
                    content::JsReplace(kWaitForTextScript,
                                       element_id,
                                       expected_text))
        .ExtractString();
```

---

<a id="TJ-002"></a>

## ✅ Manual Polling Loop (Fallback)

**Use C++ polling only when JavaScript event-driven patterns aren't applicable** (e.g., checking for element existence, waiting for JS API readiness):

```cpp
const base::TimeTicks deadline = base::TimeTicks::Now() + base::Seconds(10);
for (;;) {
  NonBlockingDelay(base::Milliseconds(10));
  if (content::EvalJs(web_contents, "!!document.getElementById('foo')",
                      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                      ISOLATED_WORLD_ID_BRAVE_INTERNAL)
          .ExtractBool()) {
    break;
  }
  if (base::TimeTicks::Now() >= deadline) {
    FAIL() << "Timeout waiting for element";
  }
}
```

---

<a id="TJ-003"></a>

## Use Isolated Worlds for Test Code

When evaluating JavaScript in tests, use `ISOLATED_WORLD_ID_BRAVE_INTERNAL` to avoid interfering with page scripts:

```cpp
content::EvalJs(web_contents, "document.getElementById('foo')",
                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                ISOLATED_WORLD_ID_BRAVE_INTERNAL)
```

---

<a id="TJ-004"></a>

## Wait for Renderer-Side JS Setup

**Problem:** Mojo binding completes before JavaScript event emitter setup.

**Example (Solana provider):**
```cpp
// ❌ WRONG - WaitForSolanaProviderBinding only waits for mojo, not JS
WaitForSolanaProviderBinding();
// window.braveSolana.on might not be ready yet!

// ✅ CORRECT - Manual polling for JS API readiness
const base::TimeTicks deadline = base::TimeTicks::Now() + base::Seconds(5);
for (;;) {
  NonBlockingDelay(base::Milliseconds(10));
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
