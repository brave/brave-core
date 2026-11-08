---
name: brave/prefer-event-driven-js-waits-over-cxx-polling
description:
  A test waiting for a DOM change evaluates one script that waits event-driven,
  such as with a MutationObserver, instead of polling EvalJs() from a C++ loop;
  C++ polling is the fallback for what has no event.
rubric: testing
severity: should_fix
---

**Rule.** When a test waits for the DOM to change, evaluate a single script that
waits in the page, event-driven: return a `Promise` that a `MutationObserver`
resolves (and disconnects) once the expected state appears, after checking
whether it already holds, as Chromium's
`service_worker_internals_ui_browsertest.cc` does. Do not poll the DOM from a
C++ loop of repeated `content::EvalJs()` calls. Poll from C++ only when no
event-driven pattern applies, such as checking that an element exists or waiting
for a JavaScript API to become ready; such a loop waits with
`NonBlockingDelay()` between checks and fails the test at a `TestTimeouts`
deadline.

**Why.** An event-driven wait responds as soon as the change happens, so it is
more deterministic, and it wastes no CPU on polling. It is also the pattern
Chromium's own tests use.

## Examples

Bad:

```cpp
// Polls the text from C++ although a DOM mutation signals the change.
const base::TimeTicks deadline =
    base::TimeTicks::Now() + TestTimeouts::action_timeout();
while (content::EvalJs(web_contents,
                       "document.getElementById('status').innerText")
           .ExtractString() != "Done") {
  NonBlockingDelay(TestTimeouts::tiny_timeout());
  ASSERT_LT(base::TimeTicks::Now(), deadline);
}
```

Good:

```cpp
static constexpr char kWaitForTextScript[] = R"(
  (function() {
    const element = document.getElementById($1);
    if (element.innerText === $2) {
      return element.innerText;
    }
    return new Promise(function(resolve) {
      const observer = new MutationObserver(function() {
        if (element.innerText === $2) {
          observer.disconnect();
          resolve(element.innerText);
        }
      });
      observer.observe(element,
          {childList: true, subtree: true, characterData: true});
    });
  })()
)";
EXPECT_EQ("Done", content::EvalJs(web_contents,
                                  content::JsReplace(kWaitForTextScript,
                                                     "status", "Done")));
```
