---
name: brave/date-obsolete-pref-migrations
description:
  'Each entry added to the obsolete-pref migration code (registration for
  migration, ClearPref()) is annotated with the date it was added.'
rubric: readability
severity: nit
---

**Rule.** When a change adds a deprecated pref to the migration code (its
registration in `RegisterProfilePrefsForMigration()` or the local state
equivalent, and its `ClearPref()` in the migration function), it adds a comment
with the date, such as `// Added 2025-01`, on or just above the entry.

**Why.** Migration entries can be deleted once enough releases have passed for
every profile to have run them. The date is what tells a later cleanup which
entries are old enough to remove.

## Examples

Bad:

```cpp
profile_prefs->ClearPref(kOldFeaturePref);
```

Good:

```cpp
// Added 2025-01 (safe to remove after ~3 releases)
profile_prefs->ClearPref(kOldFeaturePref);
```
