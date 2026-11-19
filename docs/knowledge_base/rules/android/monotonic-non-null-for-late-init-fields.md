---
name: brave/monotonic-non-null-for-late-init-fields
description:
  A field that starts null but is never set back to null after initialization is
  declared @MonotonicNonNull, not @Nullable.
rubric: correctness
severity: should_fix
---

**Rule.** Declare a field that starts as null and, once assigned, is never set
back to null as `@MonotonicNonNull`, not `@Nullable`. Keep `@Nullable` for
fields that can become null again.

**Why.** With `@MonotonicNonNull`, NullAway trusts the field to stay non-null
after its first assignment, including inside lambdas that run later, so uses
after initialization need no null checks or `assumeNonNull()`.

## Examples

Bad:

```java
private @Nullable String mSomeValue;

public void doThing(String value) {
    mSomeValue = value;
    PostTask.postTask(TaskTraits.USER_BLOCKING, () -> helper(assumeNonNull(mSomeValue)));
}
```

Good:

```java
private @MonotonicNonNull String mSomeValue;

public void doThing(String value) {
    mSomeValue = value;
    PostTask.postTask(TaskTraits.USER_BLOCKING, () -> helper(mSomeValue));
}
```
