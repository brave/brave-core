---
name: brave/visible-for-testing-on-test-only-accessors
description:
  A field or method made package-private only for tests is annotated
  @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE).
rubric: readability
severity: should_fix
---

**Rule.** When a field or method is package-private (rather than private) solely
so that tests can reach it, it MUST be annotated
`@VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)`.

**Why.** The annotation tells other developers that the wider visibility is for
tests only, and lets IDEs and lint flag uses of the member from non-test code.

## Examples

Bad:

```java
// Package-private so that FooUtilsTest can call it.
static boolean isFooValid(String foo) { ... }
```

Good:

```java
@VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
static boolean isFooValid(String foo) { ... }
```
