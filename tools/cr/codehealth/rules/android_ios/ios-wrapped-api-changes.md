---
name: android_ios/ios-wrapped-api-changes
rubric: android_ios
severity: deprecation
---

**Rule.** `brave/ios` follows the deprecations and return-type changes of the
`ios/web` and `ios/chrome` APIs it wraps.

**Why.** Wrapped APIs change under Brave's bridges, and the iOS build is the
only thing that notices, often late.

**How to check.** For each candidate commit, grep `brave/ios` for the API it
deprecates or changes, e.g. `web::ScriptMessage::legacy_body()`, or the
`ios::*Factory::GetForProfile()` whose return type changed.

## Examples

- M157: `HostContentSettingsMap` became a `scoped_refptr`.
