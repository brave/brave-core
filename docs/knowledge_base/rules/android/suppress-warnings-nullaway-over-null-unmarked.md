---
name: brave/suppress-warnings-nullaway-over-null-unmarked
description:
  A NullAway false positive inside a method is silenced with
  @SuppressWarnings("NullAway"); @NullUnmarked is only for classes and methods
  not yet migrated to null safety.
rubric: correctness
severity: should_fix
---

**Rule.** To silence a NullAway false positive inside a method, use
`@SuppressWarnings("NullAway")`. Use `@NullUnmarked` only for classes or methods
that are not yet migrated to null safety (or to match an upstream method that
is, see
[brave/match-upstream-nullability-in-overrides](match-upstream-nullability-in-overrides.md)).

**Why.** Both suppress the warnings in the method body, but
`@SuppressWarnings("NullAway")` leaves the method `@NullMarked`, so callers keep
full null checking, while `@NullUnmarked` gives its parameters and return type
unknown nullability, so callers lose null checking too.

## Examples

Bad:

```java
@NullUnmarked
public String getTitle() {
    return mTitles.get(mIndex);  // False positive.
}
```

Good:

```java
@SuppressWarnings("NullAway")
public String getTitle() {
    return mTitles.get(mIndex);  // False positive.
}
```
