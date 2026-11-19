---
name: brave/swift-public-over-open
description:
  'Swift declarations are public by default; open is used only when other
  modules must subclass or override them.'
rubric: design
severity: should_fix
---

**Rule.** Make a class or member that other modules use `public`. Use `open`
only when code in another module needs to subclass the class or override the
member.

**Why.** `open` makes subclassing and overriding part of the module's API, which
exposes implementation details and constrains every later change to the type;
`public` exposes only what callers use.

## Examples

Bad:

```swift
open class PlaylistManager {
  open func reload() { ... }
}
```

Good:

```swift
public class PlaylistManager {
  public func reload() { ... }
}
```
