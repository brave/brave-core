---
name: brave/ios-brave-design-system-colors
description:
  'iOS UI colors come from the Brave design system via
  UIColor(braveSystemName:), not hard-coded values; system colors are acceptable
  in settings-like screens.'
rubric: design
severity: should_fix
---

**Rule.** Take UI colors from the Brave design system:
`UIColor(braveSystemName:)` in UIKit, `Color(braveSystemName:)` in SwiftUI. Do
not hard-code colors (`UIColor(red:green:blue:alpha:)`, `UIColor(white:)`,
`Color(red:green:blue:)`, hex values, `#colorLiteral`). iOS system colors
(`.systemBackground`, `.label`, `.secondarySystemGroupedBackground`) are
acceptable in settings-like screens meant to match iOS's default styling; NEVER
flag them there.

**Why.** Design system colors keep the UI consistent with Brave's design and
adapt to dark mode; hard-coded values do neither.

## Examples

Bad:

```swift
view.backgroundColor = UIColor(red: 0.2, green: 0.4, blue: 0.8, alpha: 1)
```

Good:

```swift
view.backgroundColor = UIColor(braveSystemName: .primary20)
```
