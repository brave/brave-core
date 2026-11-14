---
name: brave/swiftui-background-in-shape
description:
  'A SwiftUI background confined to a shape uses .background(_:in:), not
  .background() followed by .clipShape().'
rubric: readability
severity: nit
---

**Rule.** When a view's background should fill a shape, pass the shape to
`.background(_:in:)` (`.background(.blue, in: .rect(cornerRadius: 8))`). Do not
chain `.background(...)` with a `.clipShape(...)` only to shape the background.

**Why.** The combined modifier states the intent in one call, and does not clip
the view's own content, overlays and shadows along with the background.

## Examples

Bad:

```swift
Text("Hello")
  .background(Color.blue)
  .clipShape(RoundedRectangle(cornerRadius: 8))
```

Good:

```swift
Text("Hello")
  .background(.blue, in: .rect(cornerRadius: 8))
```
