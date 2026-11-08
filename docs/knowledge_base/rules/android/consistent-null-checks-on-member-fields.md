---
name: brave/consistent-null-checks-on-member-fields
description:
  A member field that is null-checked on some code paths must be null-checked on
  every path that uses it, unless it is unconditionally initialized, in which
  case the checks go.
rubric: correctness
severity: should_fix
---

**Rule.** If a member field (such as a View reference) is checked for null on
some code paths, every other path that dereferences it MUST check it too. When
the field is unconditionally initialized in a constructor or its field
initializer, it is never null: remove the checks from every path instead of
adding more.

**Why.** A null check on one path says the field can be null; a path that
dereferences it without one is then a crash waiting for that case. Checks on a
field that can never be null are dead code that misleads readers about it.

## Examples

Bad:

```java
private void showSplash() {
    if (mSplashContainer != null) {
        mSplashContainer.setVisibility(View.VISIBLE);
    }
}

private void hideSplash() {
    mSplashContainer.setVisibility(View.GONE);
}
```

Good:

```java
private void hideSplash() {
    if (mSplashContainer != null) {
        mSplashContainer.setVisibility(View.GONE);
    }
}
```
