---
name: brave/early-returns-over-nested-ifs-in-java
description:
  When a condition decides whether the rest of a Java method runs, the method
  returns early instead of wrapping that rest in nested if blocks.
rubric: readability
severity: nit
---

**Rule.** When a condition determines whether the rest of a method executes,
test it and return early (`if (isFinished) return;`) instead of wrapping the
rest of the method in an `if` block, and especially in nested ones.

**Why.** Early returns keep the main logic at the top level of the method: the
nesting depth goes down and each precondition reads on its own line.

## Examples

Bad:

```java
private void handleState() {
    if (!isFinished) {
        if (hasData) {
            processData();
        }
    }
}
```

Good:

```java
private void handleState() {
    if (isFinished) return;
    if (!hasData) return;
    processData();
}
```
