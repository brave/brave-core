---
name: brave/remove-unused-java-interfaces-and-helpers
description:
  Interfaces, listener plumbing and helper methods that nothing implements or
  calls are removed before merging.
rubric: readability
severity: should_fix
---

**Rule.** A change MUST NOT add, or leave behind, an interface, a listener
pattern (listener interface, field, setter) or a helper method that nothing
implements or calls. Scaffolding from development that ended up unused is
removed before merging.

**Why.** Unused code is read, maintained and kept compiling for nothing, and
suggests to the next reader a mechanism that does not exist.

## Examples

Bad:

```java
public interface OnAnimationCompleteListener {
    void onAnimationComplete();
}

private @Nullable OnAnimationCompleteListener mListener;

public void setOnAnimationCompleteListener(OnAnimationCompleteListener listener) {
    mListener = listener;
}
```

Good:

```java
// The listener is removed: nothing implements or calls it.
```
