---
name: brave/suppress-nullaway-on-destroy
description:
  In simple cases, fields that destroy() nulls out stay non-null, and destroy()
  is annotated @SuppressWarnings("NullAway").
rubric: correctness
severity: should_fix
---

**Rule.** When a class's `destroy()` nulls out otherwise non-null fields and the
class is simple (nothing uses the fields after destruction), keep the fields
non-null and annotate `destroy()` with `@SuppressWarnings("NullAway")`. When
methods can run after destruction, use
[brave/nullable-fields-with-is-destroyed-guard](nullable-fields-with-is-destroyed-guard.md)
instead.

**Why.** In a simple class the fields are non-null for their whole useful life;
suppressing the one warning in `destroy()` says so without `@Nullable` and null
checks at every use.

## Examples

Bad:

```java
private @Nullable SomeService mService;  // Null only once destroyed.

public void doWork() {
    assumeNonNull(mService).execute();
}

public void destroy() {
    mService = null;
}
```

Good:

```java
private SomeService mService;  // stays non-null

@SuppressWarnings("NullAway")
public void destroy() {
    mService = null;
}
```
