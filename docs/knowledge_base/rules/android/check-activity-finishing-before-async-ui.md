---
name: brave/check-activity-finishing-before-async-ui
description:
  An async callback, animation listener or lambda that shows a dialog, starts an
  activity or changes views must first return if
  isActivityFinishingOrDestroyed() is true.
rubric: correctness
severity: blocking
---

**Rule.** In an Activity, every async callback, animation listener or lambda
that performs a UI operation (showing a dialog, starting an activity,
manipulating a view) MUST start with
`if (isActivityFinishingOrDestroyed()) return;`. This includes helpers such as
`maybeRequestDefaultBrowser()` that only run from such a path.

**Why.** An Activity can be destroyed between when a callback is scheduled and
when it executes. UI work done on a finishing or destroyed Activity then acts on
a window that no longer exists, and crashes or leaks.

## Examples

Bad:

```java
animator.addListener(new AnimatorListenerAdapter() {
    @Override
    public void onAnimationEnd(Animator animation) {
        mSplashContainer.setVisibility(View.GONE);
        showPager();
    }
});
```

Good:

```java
animator.addListener(new AnimatorListenerAdapter() {
    @Override
    public void onAnimationEnd(Animator animation) {
        if (isActivityFinishingOrDestroyed()) return;
        mSplashContainer.setVisibility(View.GONE);
        showPager();
    }
});
```
