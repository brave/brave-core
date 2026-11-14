---
name: brave/nullaway-method-annotations-for-conditions
description:
  Nullability that crosses a method boundary is stated with @RequiresNonNull,
  @EnsuresNonNull, @EnsuresNonNullIf or @Contract, as NullAway analyzes each
  method on its own.
rubric: correctness
severity: should_fix
---

**Rule.** When a method relies on, or establishes, the nullness of a field or
parameter for its callers, say so with a method annotation instead of
re-checking or suppressing at each use:

- `@RequiresNonNull("mField")` on a method that may assume the field is
  non-null; it only makes sense on private methods.
- `@EnsuresNonNull("mField")` on a method after which the field is non-null.
- `@EnsuresNonNullIf("mField")` (with `result = false` for a negated result) on
  a boolean method whose result says whether the field is non-null.
- `@Contract` in its supported forms, such as `"null -> false"` or
  `"_, !null -> !null"`. NullAway does not validate contracts, but assumes them
  true for callers, so they MUST be correct.

**Why.** NullAway analyzes each method on its own, so a check or assignment in a
helper says nothing to the caller unless an annotation carries it across the
method boundary.

## Examples

Bad:

```java
private boolean isThingEnabled() {
    return mThing != null;
}

void use() {
    if (isThingEnabled()) {
        assumeNonNull(mThing).run();
    }
}
```

Good:

```java
@EnsuresNonNullIf("mThing")
private boolean isThingEnabled() {
    return mThing != null;
}

void use() {
    if (isThingEnabled()) {
        mThing.run();
    }
}
```
