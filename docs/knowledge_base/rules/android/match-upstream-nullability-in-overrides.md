---
name: brave/match-upstream-nullability-in-overrides
description:
  A Brave Java method overriding an upstream one carries the same nullability
  annotations as the upstream method, such as @NullUnmarked, @Nullable or
  @NonNull.
rubric: correctness
severity: should_fix
---

**Rule.** When Brave Java code overrides an upstream method, the override MUST
carry the same nullability annotations as the upstream declaration:
`@NullUnmarked` on the method when upstream has it, and `@Nullable` or
`@NonNull` on the same parameters and return type.

**Why.** NullAway checks overrides against the method they override; mismatched
annotations fail the NullAway build step, which prevents merging.

## Examples

Bad:

```java
// The upstream onResult() is @NullUnmarked.
@Override
public void onResult(Profile profile) { ... }
```

Good:

```java
@NullUnmarked
@Override
public void onResult(Profile profile) { ... }
```
