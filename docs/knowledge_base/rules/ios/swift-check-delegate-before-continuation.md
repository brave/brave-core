---
name: brave/swift-check-delegate-before-continuation
description:
  'Before entering a continuation that a delegate callback resumes, Swift code
  verifies that the delegate is set.'
rubric: correctness
severity: blocking
---

**Rule.** When a `withCheckedContinuation` (or its throwing or unsafe variants)
is resumed from a delegate callback, check that the delegate is set (and the
callback can be delivered) before entering the continuation; if it is not,
return or throw instead of awaiting.

**Why.** Without a delegate the callback never arrives, so the continuation
never resumes and the awaiting task hangs forever.

## Examples

Bad:

```swift
func authenticate() async -> Bool {
  await withCheckedContinuation { continuation in
    self.continuation = continuation
    session.start()  // Resumed by the delegate, which may be nil.
  }
}
```

Good:

```swift
func authenticate() async -> Bool {
  guard session.delegate != nil else { return false }
  return await withCheckedContinuation { continuation in
    self.continuation = continuation
    session.start()
  }
}
```
