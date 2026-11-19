---
name: brave/no-pointers-to-pref-values
description:
  'Code must not store a pointer or reference to a base::Value returned by
  PrefService; clone it for longer-lived use.'
rubric: safety
severity: blocking
---

**Rule.** NEVER keep a pointer or reference to a `base::Value`,
`base::Value::Dict` or `base::Value::List` returned by a `PrefService` beyond
its immediate use (in a member, or in a local that outlives the next pref
update). Use the value inline when it is needed only briefly, or `Clone()` it
for longer-lived use.

**Why.** The value lives in the pref store. When the preference is updated or
the `PrefService` destroyed, the pointer dangles and a later use can crash.

## Examples

Bad:

```cpp
const base::Value::Dict* dict = &pref_service->GetDict(kMyPref);
// ... later use of `dict` may crash
```

Good:

```cpp
base::Value::Dict dict = pref_service->GetDict(kMyPref).Clone();

if (pref_service->GetDict(kMyPref).FindString("key")) { ... }
```
