---
name: brave/deprecate-prefs-before-removing
description:
  'A pref that was stored in user profiles is first deprecated (registered for
  clearing) for a release before its registration is removed entirely.'
rubric: correctness
severity: should_fix
---

**Rule.** When a change removes the registration of a profile or local state
pref that has shipped, it MUST NOT simply delete it. It moves the pref to the
obsolete-pref migration (register it in the migration registration function and
`ClearPref()` it in the migration function), and the pref is removed completely
only in a later release.

**Why.** Deleting a pref's registration stops the browser from reading it, but
leaves the stored value in every existing profile forever. Registering it for
clearing first removes that stale data from users' profiles.

## Examples

Bad:

```cpp
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  // kOldFeaturePref's registration is deleted here, and its value stays in
  // every existing profile.
  registry->RegisterBooleanPref(kNewFeaturePref, false);
}
```

Good:

```cpp
void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 2025-01
  registry->RegisterBooleanPref(kOldFeaturePref, false);
}

void MigrateObsoleteProfilePrefs(PrefService* profile_prefs) {
  // Added 2025-01
  profile_prefs->ClearPref(kOldFeaturePref);
}
```
