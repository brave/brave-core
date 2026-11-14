---
name: brave/direct-null-checks-not-intermediate-booleans
description:
  A null check is written directly in the if condition, never stored in an
  intermediate boolean variable that NullAway cannot follow.
rubric: correctness
severity: should_fix
---

**Rule.** Always write a null check directly in the `if` condition that guards
the dereference (`if (thing != null)`). NEVER store it in a boolean variable
(`boolean isNull = thing == null;`) and test that instead.

**Why.** NullAway cannot track nullness through intermediate boolean variables
(NullAway#98): behind one, it still warns at the dereference, and the warning
then gets suppressed instead of fixed.

## Examples

Bad:

```java
boolean isNull = thing == null;
if (!isNull) {
    thing.doWork();
}
```

Good:

```java
if (thing != null) {
    thing.doWork();
}
```
