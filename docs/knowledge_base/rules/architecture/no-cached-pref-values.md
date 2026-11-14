---
name: brave/no-cached-pref-values
description:
  'Do not cache pref values in member variables; read the pref when the value is
  needed.'
rubric: design
severity: should_fix
---

**Rule.** Do not copy a pref's value into a member variable and keep it in step
with a pref observer. Read the pref from the `PrefService` at the point the
value is needed.

**Why.** A cached copy duplicates the pref's storage, and goes stale whenever an
update path misses it; reading the pref is cheap and always current.

## Examples

Bad:

```cpp
bool is_opted_in_ = false;
void OnPrefChanged() { is_opted_in_ = prefs_->GetBoolean(kOptedIn); }
bool IsOptedIn() { return is_opted_in_; }
```

Good:

```cpp
bool IsOptedIn() { return prefs_->GetBoolean(kOptedIn); }
```
