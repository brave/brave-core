---
name: brave/ios-size-images-explicitly
description:
  'UIImageView and SwiftUI Image views get an explicit contentMode or size
  constraints, so images do not show at their natural size.'
rubric: correctness
severity: should_fix
---

**Rule.** Give each `UIImageView` an explicit `contentMode` or size constraints,
and each SwiftUI `Image` of a bitmap `.resizable()` with a frame or aspect
ratio. Nala icons are the exception: they are sized with a font (see
`brave/swiftui-size-nala-icons-with-font`).

**Why.** Without them, the image displays at its natural pixel size, which
varies with the asset and breaks the layout around it.

## Examples

Bad:

```swift
Image("icon")

let imageView = UIImageView(image: favicon)
```

Good:

```swift
Image("icon")
  .resizable()
  .frame(maxWidth: 24, maxHeight: 24)

let imageView = UIImageView(image: favicon)
imageView.contentMode = .scaleAspectFill
```
