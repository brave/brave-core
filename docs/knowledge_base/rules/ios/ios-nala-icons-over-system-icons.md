---
name: brave/ios-nala-icons-over-system-icons
description:
  "iOS UI uses Nala (Leo) icons through Image(braveSystemName:), not Apple's SF
  Symbols through Image(systemName:)."
rubric: design
severity: should_fix
---

**Rule.** Show icons from the Nala (Leo) set: `Image(braveSystemName: "leo.*")`
in SwiftUI, `UIImage(braveSystemNamed:)` in UIKit, `braveSystemImage:` for a
`Label`. Do not use Apple's SF Symbols (`Image(systemName:)`,
`UIImage(systemName:)`, `systemImage:`).

**Why.** Nala icons keep the app visually consistent with the Brave design
system on every platform.

## Examples

Bad:

```swift
Image(systemName: "trash")
```

Good:

```swift
Image(braveSystemName: "leo.trash")
```
