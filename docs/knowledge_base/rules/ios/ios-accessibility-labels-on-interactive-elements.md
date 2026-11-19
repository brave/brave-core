---
name: brave/ios-accessibility-labels-on-interactive-elements
description:
  'Interactive iOS elements always have an accessibility label, and refactors of
  UI keep the existing labels.'
rubric: correctness
severity: blocking
---

**Rule.** Every interactive element a change adds (button, toggle, tappable
view, bar button item) MUST have an accessibility label, from its visible title
or set explicitly (`accessibilityLabel`, `.accessibilityLabel(...)`). A change
that rewrites or moves UI MUST keep the labels the old code set.

**Why.** VoiceOver users find and operate controls by their labels; an
unlabelled button reads as "button", and a label lost in a refactor silently
breaks a flow that worked.

## Examples

Bad:

```swift
let shareButton = UIButton(type: .system)
shareButton.setImage(UIImage(braveSystemNamed: "leo.share.macos"), for: .normal)
```

Good:

```swift
let shareButton = UIButton(type: .system)
shareButton.setImage(UIImage(braveSystemNamed: "leo.share.macos"), for: .normal)
shareButton.accessibilityLabel = Strings.shareActionTitle
```
