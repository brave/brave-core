---
name: brave/swiftui-stack-spacing-over-child-padding
description:
  "Even gaps between the children of a SwiftUI VStack or HStack come from the
  stack's spacing parameter, not from padding on each child."
rubric: readability
severity: nit
---

**Rule.** To put the same gap between the children of a `VStack` or `HStack`,
pass it as the stack's `spacing:` argument. Do not add `.padding(.bottom, n)`,
`.padding(.trailing, n)` or similar to each child for that purpose.

**Why.** One `spacing:` value is easier to read and change than a padding
repeated on every child, and it stays right when children are added, removed or
reordered.

## Examples

Bad:

```swift
VStack {
  Text("Title")
    .padding(.bottom, 8)
  Text("Subtitle")
    .padding(.bottom, 8)
  Text("Body")
}
```

Good:

```swift
VStack(spacing: 8) {
  Text("Title")
  Text("Subtitle")
  Text("Body")
}
```
