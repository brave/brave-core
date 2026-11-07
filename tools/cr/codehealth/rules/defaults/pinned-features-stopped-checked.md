---
name: defaults/pinned-features-stopped-checked
rubric: defaults
severity: regression
---

**Rule.** Code paths that Brave's pinned features gated still check them.

**Why.** Upstream removing a feature check makes the behaviour unconditional,
whatever Brave pins.

**How to check.** The candidates remove `IsEnabled(k…)` checks of features Brave
names. Equivalent to `git diff $A $B -U0 -G'\b(kA|kB|…)\b'` over Brave's pinned
features. Keep those Brave pins, and read what the removed check guarded.
