---
name: brave/ios-prepare-thumbnail-for-thumbnails
description:
  'Thumbnails are generated with UIImage.prepareThumbnail(of:), not by resizing
  manually with UIGraphicsImageRenderer.'
rubric: perf
severity: should_fix
---

**Rule.** To make a thumbnail of an image, call `prepareThumbnail(of:)` (or its
async `byPreparingThumbnail(ofSize:)`). Do not draw the image into a
`UIGraphicsImageRenderer` or a `UIGraphicsBeginImageContext` context at the
smaller size.

**Why.** The system method decodes and downsamples the image in one optimized
step, using less memory and time than decoding it at full size and redrawing it.

## Examples

Bad:

```swift
let renderer = UIGraphicsImageRenderer(size: size)
let thumbnail = renderer.image { _ in
  image.draw(in: CGRect(origin: .zero, size: size))
}
```

Good:

```swift
let thumbnail = image.preparingThumbnail(of: size)
```
