---
name: brave/pair-validation-guards-with-assert
description:
  A defensive null or validation guard that logs and returns is paired with an
  assert of the same condition; an invariant that always holds is asserted with
  no fallback.
rubric: correctness
severity: should_fix
---

**Rule.** A defensive null or validation check that handles the bad case
gracefully (logs with `Log.e()` and returns a fallback) MUST be preceded by an
`assert` of the expected condition, with a message, and keep its graceful
handling for release builds. A log-only guard is not enough. When the condition
is an invariant that always holds, assert it and drop the fallback branch
instead of keeping both.

**Why.** Assertions crash debug builds, which makes the problem visible at once,
while the graceful handling still protects release builds; a log line alone is
easily missed in logcat. A fallback for a condition that can never fail is dead
code that hides the invariant.

## Examples

Bad:

```java
if (contractAddress == null || contractAddress.length() < MIN_LENGTH) {
    Log.e(TAG, "Invalid contract address");
    return "";
}
```

Good:

```java
assert contractAddress != null && contractAddress.length() >= MIN_LENGTH
        : "Invalid contract address";
if (contractAddress == null || contractAddress.length() < MIN_LENGTH) {
    Log.e(TAG, "Invalid contract address");
    return "";
}
```
