---
name: brave/no-clickable-focusable-on-decorative-views
description:
  Purely decorative or display-only views, such as animation containers, are not
  made clickable or focusable.
rubric: correctness
severity: should_fix
---

**Rule.** Do not set `android:clickable="true"` or `android:focusable="true"`
(or call `setClickable(true)` or `setFocusable(true)`) on a view that is purely
decorative or only displays content, such as an animation container. Keep these
attributes for views the user interacts with.

**Why.** These attributes change how accessibility services and touch handling
treat the view: a focusable decorative view is a stop for TalkBack with nothing
to do, and a clickable one swallows touches meant for the views behind it.

## Examples

Bad:

```xml
<com.airbnb.lottie.LottieAnimationView
    android:id="@+id/splash_animation"
    android:clickable="true"
    android:focusable="true" />
```

Good:

```xml
<com.airbnb.lottie.LottieAnimationView
    android:id="@+id/splash_animation" />
```
