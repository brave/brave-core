---
name: brave/ios-nala-button-shapes
description:
  'iOS buttons use the rounded rectangle shapes of the Nala design system, not
  capsule or sharp-cornered shapes unless the design calls for them.'
rubric: design
severity: should_fix
---

**Rule.** Give buttons the rounded rectangle shape of the Nala design system. Do
not make them capsules (`Capsule()`, `.buttonBorderShape(.capsule)`,
`cornerRadius = height / 2`) or sharp-cornered unless the design for that screen
specifies it.

**Why.** Consistent button shapes are part of what makes the app look like
Brave's design system rather than a mix of styles.

## Examples

Bad:

```swift
Button("Continue") { ... }
  .padding()
  .background(Color(braveSystemName: .buttonBackground), in: Capsule())
```

Good:

```swift
Button("Continue") { ... }
  .padding()
  .background(
    Color(braveSystemName: .buttonBackground),
    in: .rect(cornerRadius: 12))
```
