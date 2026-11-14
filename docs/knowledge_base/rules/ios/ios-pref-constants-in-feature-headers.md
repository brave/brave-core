---
name: brave/ios-pref-constants-in-feature-headers
description:
  'iOS pref name constants are exported from feature-specific bridge headers,
  not from the shared pref_names_bridge.h.'
rubric: design
severity: should_fix
---

**Rule.** Export a feature's pref name constants to Swift from a header of that
feature (`ios/browser/brave_wallet/wallet_pref_names_bridge.h`). Do not add them
to the shared `ios/components/constants/pref_names_bridge.h`.

**Why.** A feature header keeps the dependency on the feature's prefs explicit
and local; the shared header couples every user of it to every feature's prefs.

## Examples

Bad:

```objc
// ios/components/constants/pref_names_bridge.h
OBJC_EXPORT NSString* const kPlaylistEnabledPrefName;
```

Good:

```objc
// ios/browser/playlist/playlist_pref_names_bridge.h
OBJC_EXPORT NSString* const kPlaylistEnabledPrefName;
```
