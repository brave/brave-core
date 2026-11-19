---
name: brave/nullable-fields-with-is-destroyed-guard
description:
  In complex cases, fields that destroy() nulls out are declared @Nullable, and
  methods guard their use with an isDestroyed() check annotated
  @EnsuresNonNullIf.
rubric: correctness
severity: should_fix
---

**Rule.** When a class's `destroy()` nulls out fields that are otherwise
non-null, and the class is complex (its methods can run after destruction),
declare those fields `@Nullable` and add a private `isDestroyed()` annotated
`@EnsuresNonNullIf(value = {"mService"}, result = false)`; methods return early
on `isDestroyed()` and then use the fields directly. This is the preferred one
of the two strategies; the other is
[brave/suppress-nullaway-on-destroy](suppress-nullaway-on-destroy.md).

**Why.** Nulling a field NullAway believes non-null is a warning, and declaring
it `@Nullable` without the guard forces null checks on every use. The annotated
guard makes the nullability true to the code while letting NullAway trust the
field after one check.

## Examples

Bad:

```java
private SomeService mService;

public void doWork() {
    mService.execute();  // Runs after destroy() too.
}

public void destroy() {
    mService = null;  // NullAway warns: mService is non-null.
}
```

Good:

```java
private @Nullable SomeService mService;

@EnsuresNonNullIf(value = {"mService"}, result = false)
private boolean isDestroyed() {
    return mService == null;
}

public void doWork() {
    if (isDestroyed()) return;
    mService.execute();
}

public void destroy() {
    mService = null;
}
```
