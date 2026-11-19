---
name: brave/use-pref-change-registrar
description:
  'Observe pref changes with a PrefChangeRegistrar, not a custom observer
  interface.'
rubric: design
severity: should_fix
---

**Rule.** To react to a pref changing, register a callback with a
`PrefChangeRegistrar` (`pref_change_registrar_.Add(pref, callback)`). Do not
define a custom observer interface or notification path for pref changes.

**Why.** `PrefChangeRegistrar` already delivers pref changes and unregisters
itself on destruction; a custom observer duplicates it and adds code that can
fall out of sync with the pref.

## Examples

Bad:

```cpp
class MyObserver : public PrefObserver {
  void OnPrefChanged(const std::string& pref_name) override;
};
```

Good:

```cpp
pref_change_registrar_.Add(
    prefs::kMyPref,
    base::BindRepeating(&MyClass::OnPrefChanged, base::Unretained(this)));
```
