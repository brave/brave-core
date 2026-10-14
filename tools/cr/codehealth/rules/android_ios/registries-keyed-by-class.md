---
name: android_ios/registries-keyed-by-class
rubric: android_ios
severity: regression
---

**Rule.** Registries and host delegates that upstream keys by exact class still
reach Brave's subclasses, and settings pages that name their parent return to
Brave's class.

**Why.** A lookup by exact class bypasses Brave's subclass swap without any
error.

**How to check.** For each candidate commit, see which class it registers or
names, and whether Brave replaces that class. Pages that name their parent,
`git grep -n 'finishCurrentSettings(this, [A-Z]' $B`, return to upstream's class
rather than Brave's.

## Examples

- Settings in a tab, and `SettingsFragmentRegistry`.
- Clearing browsing data returns to `PrivacySettings`.
