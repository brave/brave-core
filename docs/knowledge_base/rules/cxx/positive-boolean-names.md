---
name: brave/positive-boolean-names
description:
  'Booleans, predicates and prefs are named in the positive form (Enabled, not
  Disabled).'
rubric: readability
severity: should_fix
---

**Rule.** Name boolean variables, members, predicates and prefs in the positive
form: `IsTorEnabled()`, `kTorEnabled`, `is_tor_enabled`. Do not introduce names
such as `IsTorDisabled()`, `kTorDisabled` or `is_not_allowed_`; a caller that
needs the opposite negates the positive name.

**Why.** Negative names lead to double negatives (`!IsTorDisabled()`) at call
sites and in UI bindings, which are harder to read and easy to invert by
mistake. Using the positive form everywhere also keeps related names consistent.

## Examples

Bad:

```cpp
bool IsTorDisabled();
inline constexpr char kTorDisabled[] = "tor.disabled";
```

Good:

```cpp
bool IsTorEnabled();
inline constexpr char kTorEnabled[] = "tor.enabled";
```
