---
name: brave/swiftui-size-nala-icons-with-font
description:
  'Nala icons, which are custom SF Symbols, are sized with .font(), never with
  .resizable().frame().'
rubric: design
severity: should_fix
---

**Rule.** Size an `Image(braveSystemName:)` with `.font(...)` (a text style such
as `.title3`, or a scaled custom size). Do not make it `.resizable()` and give
it a `.frame(width:height:)`.

**Why.** Nala icons are custom SF Symbols, drawn like font glyphs: sized by
font, they align with text, keep their stroke weight, and scale with Dynamic
Type; stretched as images, they do none of that.

## Examples

Bad:

```swift
Image(braveSystemName: "leo.trash")
  .resizable()
  .frame(width: 24, height: 24)
```

Good:

```swift
Image(braveSystemName: "leo.trash")
  .font(.title3)
```
