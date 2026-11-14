---
name: brave/ios-register-prefs-in-browser-prefs
description:
  'iOS preferences are registered in browser_prefs.mm, not in arbitrary init
  methods.'
rubric: design
severity: should_fix
---

**Rule.** Register iOS preferences from `browser_prefs.mm`: Brave's
`RegisterBrowserStatePrefs()` and `RegisterLocalStatePrefs()` in
`ios/browser/shared/prefs/browser_prefs_impl.mm`, which the `chromium_src`
override of upstream's `browser_prefs.mm` calls. Do not register prefs from a
service constructor, a factory or another init method.

**Why.** Prefs must be registered before any profile or local state reads them;
registration in one central place guarantees that order and shows every pref
Brave adds on iOS, while registration in init methods runs late or not at all.

## Examples

Bad:

```cpp
DebounceService::DebounceService(PrefService* prefs) {
  // Too late: prefs may already have been read.
  user_prefs::PrefRegistrySyncable* registry = ...;
  registry->RegisterBooleanPref(kDebounceEnabled, true);
}
```

Good:

```cpp
// ios/browser/shared/prefs/browser_prefs_impl.mm
void RegisterBrowserStatePrefs(user_prefs::PrefRegistrySyncable* registry) {
  debounce::DebounceService::RegisterProfilePrefs(registry);
}
```
