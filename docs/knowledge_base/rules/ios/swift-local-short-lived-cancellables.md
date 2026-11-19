---
name: brave/swift-local-short-lived-cancellables
description:
  'A Combine subscription needed only briefly, such as one waiting for a single
  value, is held in a local variable, not stored as a property.'
rubric: design
severity: should_fix
---

**Rule.** When a Combine subscription only lives until one event (waiting for a
single value, a one-off completion), keep its `AnyCancellable` in a local
variable released when done. Do not store it in a property or in a long-lived
`Set<AnyCancellable>`.

**Why.** Cancellables stored for short-lived work accumulate as dead
subscriptions in the property for the lifetime of the object.

## Examples

Bad:

```swift
playerModel.$isReady
  .first(where: { $0 })
  .sink { [weak self] _ in self?.play() }
  .store(in: &cancellables)  // Kept until the controller is gone.
```

Good:

```swift
var readyCancellable: AnyCancellable?
readyCancellable = playerModel.$isReady
  .first(where: { $0 })
  .sink { [weak self] _ in
    self?.play()
    readyCancellable = nil
  }
```
