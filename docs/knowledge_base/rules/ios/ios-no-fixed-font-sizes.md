---
name: brave/ios-no-fixed-font-sizes
description:
  'iOS UI uses semantic font styles that follow Dynamic Type; a custom size is
  scaled with @ScaledMetric or UIFontMetrics, never fixed.'
rubric: correctness
severity: blocking
---

**Rule.** Set fonts with semantic text styles (`.font(.subheadline)`,
`UIFont.preferredFont(forTextStyle: .body)`). NEVER use a fixed point size
(`.font(.system(size: 15))`, `UIFont.systemFont(ofSize: 15)`) unless it is
scaled: with `@ScaledMetric` in SwiftUI, or `UIFontMetrics` in UIKit.

**Why.** Users choose their text size in the system settings; fixed sizes ignore
that choice and make the UI unreadable for those who need larger text.

## Examples

Bad:

```swift
Text("Hello World")
  .font(.system(size: 15))
```

Good:

```swift
Text("Hello World")
  .font(.subheadline)

@ScaledMetric private var customSize = 15
Text("Hello World")
  .font(.system(size: customSize))
```
