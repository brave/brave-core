---
name: brave/static-assert-after-upstream-enum-values
description:
  A Brave value placed after an upstream enum's last value is guarded with a
  static_assert against the upstream count, so new upstream values cannot clash
  with it.
rubric: correctness
severity: should_fix
---

**Rule.** When an override defines a Brave value past the end of an upstream
enum (a constant above its `kMaxValue` or `*_COUNT`, or entries appended after
its last value), add a `static_assert` comparing the upstream enum's count or
last value with the Brave value, with a message saying what to update.

**Why.** Upstream adds enum values as it goes. Without the assertion, a new
upstream value silently takes the number Brave uses, and the two collide at run
time; with it, the rebase that adds the value fails to compile.

## Examples

Bad:

```cpp
constexpr int kBravePolicySource = 10;
```

Good:

```cpp
constexpr int kBravePolicySource = 10;
static_assert(static_cast<int>(policy::POLICY_SOURCE_COUNT) <= kBravePolicySource,
              "Upstream added new policy sources - update kBravePolicySource");
```
