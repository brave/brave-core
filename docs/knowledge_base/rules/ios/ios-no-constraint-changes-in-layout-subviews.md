---
name: brave/ios-no-constraint-changes-in-layout-subviews
description:
  'Auto Layout constraints are never created or modified inside layoutSubviews.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER create, activate, deactivate or change the `constant` of an Auto
Layout constraint inside `layoutSubviews`. Set constraints once during setup,
and make later changes in `updateViewConstraints` (`updateConstraints` in a
`UIView` subclass).

**Why.** Changing a constraint invalidates the layout, which calls
`layoutSubviews` again, which changes the constraint again: an infinite layout
loop.

## Examples

Bad:

```swift
override func layoutSubviews() {
  super.layoutSubviews()
  widthConstraint.constant = bounds.width / 2
}
```

Good:

```swift
override func updateViewConstraints() {
  widthConstraint.constant = bounds.width / 2
  super.updateViewConstraints()
}
```
