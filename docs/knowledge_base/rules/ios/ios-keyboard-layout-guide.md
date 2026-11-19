---
name: brave/ios-keyboard-layout-guide
description:
  'Keyboard-responsive layouts constrain to UIView.keyboardLayoutGuide instead
  of observing keyboard notifications and adjusting insets by hand.'
rubric: design
severity: should_fix
---

**Rule.** To keep content above the keyboard, constrain it to
`view.keyboardLayoutGuide`. Do not observe `keyboardWillShowNotification`,
`keyboardWillHideNotification` or `keyboardWillChangeFrameNotification` to read
the keyboard frame and adjust constraints or insets manually.

**Why.** The layout guide tracks the keyboard's frame, including undocked,
floating and hardware keyboards, and animates with it; manual observation
repeats that logic and tends to miss some of those cases.

## Examples

Bad:

```swift
NotificationCenter.default.addObserver(self,
  selector: #selector(keyboardWillShow),
  name: UIResponder.keyboardWillShowNotification, object: nil)
```

Good:

```swift
bottomConstraint = view.bottomAnchor.constraint(
  equalTo: view.keyboardLayoutGuide.topAnchor)
```
