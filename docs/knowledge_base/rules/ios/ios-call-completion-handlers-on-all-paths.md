---
name: brave/ios-call-completion-handlers-on-all-paths
description:
  'A function that accepts a completion handler calls it on every code path,
  including error and early-return paths.'
rubric: correctness
severity: blocking
---

**Rule.** A function that takes a completion handler MUST call it on every path
out of it: each early `return`, each `guard ... else`, each error branch and
each `catch`. An added early exit in such a function MUST call the handler (with
a failure result where it takes one) before returning.

**Why.** A caller waiting on the handler waits forever when a path skips it, and
whatever it holds for the duration of the call (a spinner, a continuation,
captured resources) leaks.

## Examples

Bad:

```swift
func fetchData(from url: URL, _ completionHandler: @escaping () -> Void) {
  if url.scheme != "https" { return }  // Never calls the handler.
  // ...
}
```

Good:

```swift
func fetchData(from url: URL, _ completionHandler: @escaping () -> Void) {
  if url.scheme != "https" {
    completionHandler()
    return
  }
  // ...
}
```
