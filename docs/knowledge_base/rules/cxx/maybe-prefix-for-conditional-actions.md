---
name: brave/maybe-prefix-for-conditional-actions
description:
  'A function that performs its action only when some condition holds is named
  Maybe*.'
rubric: readability
severity: nit
---

**Rule.** Prefix the name of a function that checks a condition and may then do
nothing with `Maybe`: `MaybeShowFirstLaunchNotification()`,
`MaybeHideReferrer()`. A function named for an action without the prefix
performs it every time it is called.

**Why.** A call to `ShowFirstLaunchNotification()` reads as if it always shows
the notification; a reader debugging a missing notification would not look for
an early return inside it.

## Examples

Bad:

```cpp
// Returns early unless this is the first launch.
void ShowFirstLaunchNotification();
```

Good:

```cpp
void MaybeShowFirstLaunchNotification();
```
