---
name: brave/swift-assign-unused-results-to-underscore
description:
  'A Swift value that is intentionally unused, such as the result of a
  side-effecting call, is assigned to _ instead of a named variable.'
rubric: readability
severity: nit
---

**Rule.** When a value is computed only for a side effect and never read, write
`_ = expression` instead of binding it to a named `let` or `var`.

**Why.** `_ =` tells the reader that discarding the value is deliberate, and
silences the compiler's "never used" and "written to, but never read" warnings.

## Examples

Bad:

```swift
let removed = cache.removeValue(forKey: key)  // Never read.
```

Good:

```swift
_ = cache.removeValue(forKey: key)
```
