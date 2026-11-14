---
name: brave/no-static-per-profile-state
description:
  'Per-profile settings or state are never kept in static or global variables,
  but in profile-attached keyed services or UserData.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER store a setting or state that belongs to a profile in a `static`
variable, a function-local static or a global (`g_*`). Keep it in a keyed
service for the profile, in `UserData` attached to it, or in its prefs.

**Why.** Static state is shared by every profile in the process, including
incognito and guest profiles: one profile's setting leaks into the others, and
the last profile to write it wins.

## Examples

Bad:

```cpp
// Set from the active profile's prefs.
static bool g_shields_enabled = true;
```

Good:

```cpp
class ShieldsSettingsService : public KeyedService {
 public:
  bool shields_enabled() const { return shields_enabled_; }

 private:
  bool shields_enabled_ = true;
};
```
