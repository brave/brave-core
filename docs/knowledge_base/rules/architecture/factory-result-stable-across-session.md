---
name: brave/factory-result-stable-across-session
description:
  'A keyed service factory or its GetFor* accessor returns null only on
  session-stable attributes (profile type, buildflags, base::Feature), never on
  a user-toggleable pref.'
rubric: correctness
severity: blocking
---

**Rule.** Neither `BuildServiceInstanceForBrowserContext()` nor the static
`GetForProfile()` / `GetForBrowserContext()` accessor may return null based on
state that can change while the browser runs, such as a user-toggleable pref
(`kBraveWalletEnabledPref`, `kPlaylistEnabledPref`). Valid bases are the profile
type, `BUILDFLAG(ENABLE_*)` buildflags, `base::Feature` flags, and (short-term)
managed prefs documented to need a restart. For a feature that can be turned off
at runtime, keep building the service and give it an `IsEnabled()` accessor, or
have callers observe the pref.

**Why.** The factory decides once per profile; a decision based on a pref holds
for the rest of the session whatever the user toggles, so applying the user's
choice needs a restart, and callers start null-checking what should be a stable
handle.

## Examples

Bad:

```cpp
std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  PrefService* prefs = user_prefs::UserPrefs::Get(context);
  if (!prefs->GetBoolean(kBraveWalletEnabledPref)) {
    return nullptr;  // User toggling the pref now requires a restart
  }
  return std::make_unique<BraveWalletService>(...);
}
```

Good:

```cpp
std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<BraveWalletService>(
      user_prefs::UserPrefs::Get(context), ...);
}
```
