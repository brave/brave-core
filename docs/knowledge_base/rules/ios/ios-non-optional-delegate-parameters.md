---
name: brave/ios-non-optional-delegate-parameters
description:
  'Delegate protocol methods take non-optional parameters, so the delegate never
  handles nil from the delegating object.'
rubric: design
severity: should_fix
---

**Rule.** Declare the parameters of a delegate protocol's methods as
non-optional: no `Item?` in Swift, no `nullable` in Objective-C.

**Why.** An optional parameter pushes the delegating object's state handling
into every delegate, each of which has to decide what `nil` means.

## Examples

Bad:

```swift
protocol TabTrayDelegate: AnyObject {
  func tabTray(_ tabTray: TabTrayController, didSelect tab: TabState?)
}
```

Good:

```swift
protocol TabTrayDelegate: AnyObject {
  func tabTray(_ tabTray: TabTrayController, didSelect tab: TabState)
}
```
