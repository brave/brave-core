---
name: defaults/prefs-upstream-stops-reading
rubric: defaults
severity: regression
---

**Rule.** Upstream still reads each pref Brave sets a default for or overrides.

**Why.** Brave's `SetDefaultPrefValue` on a pref upstream no longer reads keeps
nothing off.

**How to check.** The candidates remove reads or registrations of prefs Brave
names. Check each against Brave's `SetDefaultPrefValue` and pref overrides
(`git grep -n '<pref>' -- '*.cc'`).
