---
name: brave/annotate-native-bound-parameter-nullability
description:
  Parameters of Java-to-native methods and of @CalledByNative methods carry
  their true nullability annotations, as JNI boundaries are checked only through
  them or not at all.
rubric: correctness
severity: should_fix
---

**Rule.** Parameters of methods that cross the JNI boundary MUST carry their
correct nullability annotations. Mark a `@NativeMethods` (Java-to-native)
parameter `@Nullable` exactly when native accepts null, so Java callers are
checked against it. For `@CalledByNative` methods, whose nullness NullAway does
not check, annotate parameters native can pass as null with `@Nullable` and
handle that case.

**Why.** Java-to-native calls in `@NullMarked` code are checked with generated
`assert` statements based on these annotations, so a wrong annotation either
lets null through or asserts on a valid null. Nullness of `@CalledByNative`
methods is not checked at all (crbug/389192501), so their annotations are the
only record of what native may pass.

## Examples

Bad:

```java
@NativeMethods
interface Natives {
    // Native treats an empty or null URL as "no favicon".
    void setFavicon(long nativeFooBridge, String url);
}
```

Good:

```java
@NativeMethods
interface Natives {
    void setFavicon(long nativeFooBridge, @Nullable String url);
}
```
