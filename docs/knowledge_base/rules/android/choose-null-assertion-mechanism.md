---
name: brave/choose-null-assertion-mechanism
description:
  'Null assertions use the mechanism that fits: assumeNonNull() when sure,
  assertNonNull() for an asserting expression, assert when not dereferencing
  locally, Objects.requireNonNull() when null must stop production.'
rubric: correctness
severity: should_fix
---

**Rule.** Pick the null assertion by what the code needs:

- `assumeNonNull(x)` when you are confident the value is non-null, as a
  statement or an expression; it never crashes in production.
- `assertNonNull(x)` when you need an asserting expression (it crashes only
  where asserts run, as dump-without-crashing on Canary), and
  `assertNonNull(null)` for unreachable code paths such as a `switch` default.
- `assert x != null;` when the code does not dereference the value locally.
- `Objects.requireNonNull(x)` when a null would cause worse problems later and
  must stop production too.

Prefer the statement form for preconditions, which keeps them apart from the
use; use the expression form when it reads better.

**Why.** The mechanisms differ in form and in what they do in production. The
wrong one either crashes users where a silent assumption was intended, or lets a
null that must never pass through continue in release builds.

## Examples

Bad:

```java
public String describe(@MyIntDef int validity) {
    return switch (validity) {
        case MyIntDef.VALID -> "okay";
        case MyIntDef.INVALID -> "not okay";
        default -> Objects.requireNonNull(null);
    };
}
```

Good:

```java
public String describe(@MyIntDef int validity) {
    return switch (validity) {
        case MyIntDef.VALID -> "okay";
        case MyIntDef.INVALID -> "not okay";
        default -> assertNonNull(null);
    };
}
```
