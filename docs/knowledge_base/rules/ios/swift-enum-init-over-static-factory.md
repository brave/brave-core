---
name: brave/swift-enum-init-over-static-factory
description:
  'A Swift enum built from other values gets a convenience initializer, not a
  static factory method.'
rubric: readability
severity: nit
---

**Rule.** To create an enum value from other values, add an initializer on the
enum (`init(isPrivate: Bool) { self = ... }`), not a static method returning the
enum (`static func from(isPrivate:) -> TabType`).

**Why.** An initializer is the idiomatic Swift way to construct a value, so
callers write `TabType(isPrivate: true)` like any other type, and it integrates
better with the type system.

## Examples

Bad:

```swift
extension TabType {
  static func from(isPrivate: Bool) -> TabType {
    isPrivate ? .private : .regular
  }
}
```

Good:

```swift
extension TabType {
  init(isPrivate: Bool) {
    self = isPrivate ? .private : .regular
  }
}
```
