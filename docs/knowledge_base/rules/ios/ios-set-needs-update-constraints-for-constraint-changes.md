---
name: brave/ios-set-needs-update-constraints-for-constraint-changes
description:
  'Code that needs constraints to change calls setNeedsUpdateConstraints(), not
  setNeedsLayout() followed by layoutIfNeeded().'
rubric: correctness
severity: should_fix
---

**Rule.** When a state change requires different constraints, call
`setNeedsUpdateConstraints()` and update the constraints in
`updateConstraints`/`updateViewConstraints`. Do not force the change through
with `setNeedsLayout()` and `layoutIfNeeded()`.

**Why.** The constraint system handles layout invalidation itself once the
constraints are updated, so forcing a layout pass is redundant work.

## Examples

Bad:

```swift
isExpanded = true
setNeedsLayout()
layoutIfNeeded()
```

Good:

```swift
isExpanded = true
setNeedsUpdateConstraints()
```
