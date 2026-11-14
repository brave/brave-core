---
name: brave/swift-weak-self-on-outermost-closure
description:
  'In Swift, [weak self] goes on the outermost closure that captures self, not
  only on an inner closure nested in it.'
rubric: safety
severity: should_fix
---

**Rule.** Put `[weak self]` on the root (outermost) closure of a nested chain. A
`[weak self]` written only on an inner closure does not prevent the retain
cycle, and once the outer closure captures weakly the inner closures need no
capture list of their own.

**Why.** An inner closure captures `self` from its enclosing closure. When the
outer closure has no `[weak self]`, it captures `self` strongly so that it can
hand `self` to the inner one, and that strong capture is the one that keeps
`self` alive.

## Examples

Bad:

```swift
manager.onChange = {
  self.queue.async { [weak self] in
    self?.reload()
  }
}
```

Good:

```swift
manager.onChange = { [weak self] in
  self?.queue.async {
    self?.reload()
  }
}
```
