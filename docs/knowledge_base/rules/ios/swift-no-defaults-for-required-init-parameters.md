---
name: brave/swift-no-defaults-for-required-init-parameters
description:
  'Swift initializer parameters that callers should always specify have no
  default value.'
rubric: correctness
severity: should_fix
---

**Rule.** Do not give an initializer parameter a default value (`= false`,
`= nil`, `= .regular`, `= []`) when each caller has to decide its value, such as
whether a tab is private or which profile a service belongs to. Keep defaults
for parameters where one value is right for nearly every caller.

**Why.** A default lets a caller omit the argument without noticing, and the
object is then silently created with a value that may be wrong for that caller.

## Examples

Bad:

```swift
init(url: URL, isPrivate: Bool = false) { ... }

let tab = TabState(url: url)  // Private tabs created as regular ones.
```

Good:

```swift
init(url: URL, isPrivate: Bool) { ... }

let tab = TabState(url: url, isPrivate: isPrivate)
```
