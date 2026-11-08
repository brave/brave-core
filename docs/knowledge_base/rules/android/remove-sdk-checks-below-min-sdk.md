---
name: brave/remove-sdk-checks-below-min-sdk
description:
  Android version checks for API levels below the app's minimum SDK are removed
  as dead code.
rubric: readability
severity: should_fix
---

**Rule.** Do not add, and remove when touched, `Build.VERSION.SDK_INT` checks,
`@RequiresApi` or `@TargetApi` annotations and similar version gates for API
levels below Brave's minimum SDK (such as Lollipop, API 21, or Marshmallow, API
23): the condition is always true, so keep only the branch for newer versions.

**Why.** Brave's minimum SDK is higher than those levels, so the check can never
fail and its other branch is dead code.

## Examples

Bad:

```java
if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
    window.setStatusBarColor(color);
}
```

Good:

```java
window.setStatusBarColor(color);
```
