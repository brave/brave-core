---
name: brave/const-raw-ref-for-non-null-fields
description:
  'A non-owning field that must never be null is const raw_ref<T>; raw_ptr<T> is
  for fields that can be null, and plain raw_ref<T> only for rebinding.'
rubric: readability
severity: should_fix
---

**Rule.** A non-owning field that must never be null is a `const raw_ref<T>`,
typically initialized from a `T&` constructor parameter. `raw_ptr<T>` is for
fields that can be null or are reassigned. Use a plain (non-`const`)
`raw_ref<T>` only in the rare case where the field must never be null but must
be rebound to a different referent after construction.

**Why.** `const` prevents rebinding, matching `T&` semantics, and `raw_ref<T>`
states that the referenced object is expected to outlive the holder, so the type
tells readers the field is always set.

## Examples

Bad:

```cpp
class BraveBrowserDelegate {
 public:
  explicit BraveBrowserDelegate(BrowserWindowInterface& window)
      : window_(&window) {}

 private:
  raw_ptr<BrowserWindowInterface> window_;  // never null
};
```

Good:

```cpp
class BraveBrowserDelegate {
 public:
  explicit BraveBrowserDelegate(BrowserWindowInterface& window)
      : window_(window) {}

 private:
  const raw_ref<BrowserWindowInterface> window_;
};
```
