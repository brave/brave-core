---
name: brave/checked-companion-for-effectively-non-null-returns
description:
  A method that can return null but usually does not keeps its @Nullable return;
  callers that know it is non-null get a "Checked" companion, never a @NonNull
  mis-annotation.
rubric: correctness
severity: should_fix
---

**Rule.** When a method is technically `@Nullable` but in practice almost always
returns non-null (as `Activity.findViewById()` and `Context.getSystemService()`
do), keep its return type `@Nullable` and, in Chromium-authored code, add a
companion `fooChecked()` that returns `assertNonNull(foo())` for callers that
know the value exists. NEVER annotate the method `@NonNull`, or drop its
`@Nullable`, because callers expect non-null.

**Why.** A `@NonNull` annotation on a method that can return null hides real
nullability, so NullAway stops checking the callers that can see the null; that
defeats the purpose of the static analysis. The companion keeps the honest
signature and puts the assertion where the assumption is made.

## Examples

Bad:

```java
// Returns null when the tab is gone.
public @NonNull Tab getTabById(String tabId) { ... }
```

Good:

```java
public @Nullable Tab getTabById(String tabId) { ... }

public Tab getTabByIdChecked(String tabId) {
    return assertNonNull(getTabById(tabId));
}
```
