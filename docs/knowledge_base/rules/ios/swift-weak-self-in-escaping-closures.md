---
name: brave/swift-weak-self-in-escaping-closures
description:
  'In Swift, closures that outlive the current scope must capture [weak self];
  fire-and-forget Task {} blocks that are not stored need not.'
rubric: safety
severity: blocking
---

**Rule.** A Swift closure that outlives the current scope (a network callback, a
notification handler, an animation completion, a closure stored in a property,
or a `Task` stored in a property) MUST capture `[weak self]` and use `self?` or
`guard let self`. Use `[unowned self]` only when the closure's lifetime is
guaranteed to be shorter than `self`'s. NEVER ask for `[weak self]` under this
rule in a fire-and-forget `Task {}` that is not stored anywhere: it creates no
retain cycle.

**Why.** A stored or long-lived closure that captures `self` strongly keeps
`self` alive, and when `self` also holds the closure the two leak together. An
unstored `Task` releases its captures when it finishes, so weak capture there
only adds noise.

## Examples

Bad:

```swift
dataSource.onUpdate = {
  self.reloadData()  // Retain cycle.
}

fetchTask = Task {
  await self.fetchData()  // Stored Task: retain cycle.
}
```

Good:

```swift
dataSource.onUpdate = { [weak self] in
  self?.reloadData()
}

fetchTask = Task { [weak self] in
  await self?.fetchData()
}

// Fire-and-forget Task: no retain cycle.
Task {
  await self.fetchData()
}
```
