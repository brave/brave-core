---
name: brave/pref-keys-correct-before-shipping
description:
  'Check the spelling of each new persisted preference key in review, since
  renaming a key after it ships needs a migration.'
rubric: correctness
severity: blocking
---

**Rule.** When a change adds a persisted preference key (a
`kFoo[] = "brave.foo.bar"` constant registered with `Register*Pref()`), check
that the key string is spelled correctly and consistently with its neighbours
before it ships. A key that has already shipped MUST NOT be renamed without a
migration.

**Why.** The key is written to users' profiles; correcting it after a release
needs a migration, or the users' stored values are lost.

## Examples

Bad:

```cpp
inline constexpr char kFooEnabeld[] = "brave.foo.enabeld";
```

Good:

```cpp
inline constexpr char kFooEnabled[] = "brave.foo.enabled";
```
