---
name: brave/swiftui-label-for-icon-only-buttons
description:
  'Icon-only SwiftUI buttons use a Label with .labelStyle(.iconOnly), not a bare
  Image, so they carry an accessibility label.'
rubric: correctness
severity: blocking
---

**Rule.** The label of an icon-only SwiftUI `Button` MUST be a `Label` with a
title and the icon, styled `.labelStyle(.iconOnly)`, never a bare `Image`.

**Why.** The `Label`'s title becomes the button's accessibility label, so
VoiceOver reads its purpose; a bare `Image` leaves the button unlabelled or
reads the icon's file name.

## Examples

Bad:

```swift
Button {
  // ...
} label: {
  Image(braveSystemName: "leo.trash")
}
```

Good:

```swift
Button {
  // ...
} label: {
  Label("Delete", braveSystemImage: "leo.trash")
    .labelStyle(.iconOnly)
}
```
