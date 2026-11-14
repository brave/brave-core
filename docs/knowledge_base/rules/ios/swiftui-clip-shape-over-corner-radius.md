---
name: brave/swiftui-clip-shape-over-corner-radius
description:
  'SwiftUI views round their corners with .clipShape(.rect(cornerRadius:)), not
  the deprecated .cornerRadius() modifier.'
rubric: readability
severity: should_fix
---

**Rule.** Do not add the SwiftUI `.cornerRadius(_:)` modifier. Clip the view
with `.clipShape(.rect(cornerRadius:))` instead. This concerns the SwiftUI
modifier, not UIKit's `layer.cornerRadius` property.

**Why.** `.cornerRadius()` is deprecated; `.clipShape(.rect(cornerRadius:))` is
its replacement and also accepts other shapes and corner styles.

## Examples

Bad:

```swift
Text("Hello")
  .cornerRadius(8)
```

Good:

```swift
Text("Hello")
  .clipShape(.rect(cornerRadius: 8))
```
