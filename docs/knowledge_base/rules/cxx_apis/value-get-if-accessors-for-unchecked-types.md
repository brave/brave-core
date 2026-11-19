---
name: brave/value-get-if-accessors-for-unchecked-types
description:
  'When a base::Value may not hold the expected type, read it with
  GetIfBool()/GetIfInt()/GetIfString()/GetIfDict() and friends, not with Get*(),
  which CHECK-fails on a type mismatch.'
rubric: correctness
severity: blocking
---

**Rule.** When a `base::Value` comes from data whose type is not guaranteed
(parsed JSON, prefs written by older versions, IPC, component or server data),
read it with `GetIfBool()`, `GetIfInt()`, `GetIfDouble()`, `GetIfString()`,
`GetIfDict()` or `GetIfList()` and handle the empty result, for example with
`.value_or()`. Calling `GetBool()`, `GetInt()`, `GetString()`, `GetDict()` and
the like is only acceptable where the type has just been checked (`is_bool()`,
...) or is guaranteed by construction.

**Why.** The `Get*()` accessors `CHECK` the type, so a value of an unexpected
type crashes the browser. Untrusted or versioned data can always have an
unexpected type.

## Examples

Bad:

```cpp
// Crashes if the value is not a bool.
if (value.GetBool()) {
  Enable();
}
```

Good:

```cpp
if (value.GetIfBool().value_or(false)) {
  Enable();
}
```
