---
name: brave/swift-let-for-single-assignment-properties
description:
  'Swift class properties assigned once and never mutated are let, not var; var
  properties of structs are never flagged.'
rubric: readability
severity: nit
---

**Rule.** Declare a stored property of a class or actor with `let` when it is
assigned once (at its declaration or in the initializer) and never reassigned.
NEVER flag a `var` property of a `struct` under this rule: a struct's mutability
is decided where the value is held, by `let` or `var`. Properties that must be
`var` (`lazy`, `weak`, computed, or wrapped by `@Published`, `@State` and
similar) are not concerned.

**Why.** The compiler enforces a `let`, so an accidental reassignment fails to
build instead of changing the object's state.

## Examples

Bad:

```swift
final class DownloadController {
  var profile: ProfileIOS  // Set in init, never reassigned.

  init(profile: ProfileIOS) {
    self.profile = profile
  }
}
```

Good:

```swift
final class DownloadController {
  let profile: ProfileIOS

  init(profile: ProfileIOS) {
    self.profile = profile
  }
}

struct Item {
  var name: String  // OK: mutability controlled by usage.
}
```
