---
name: android_ios/default-flips-to-new-ui-class
rubric: android_ios
severity: regression
---

**Rule.** A feature default that switches upstream to a new UI class, or removes
preferences, keeps Brave's hides and reorders working, and Java cached flags
match Brave's C++ pins.

**Why.** Brave's hide of the legacy class doesn't apply to the new one. A flip
can also remove preferences that Brave reorders by key: upstream's private
`removePreferenceIfPresent` bypasses Brave's `findPreference` fallback, and
`assumeNonNull` doesn't stop the NPE. A Java `CachedFlag` keeps its own default,
so it needs a `BraveCachedFlag` override as well as the C++ pin.

**How to check.** For each candidate commit, find the class the flag switches to
and grep Brave for its hide of the old one; check Brave's
`BraveMainPreferencesBase`-style reorders for the removed keys; and for changed
`CachedFlag(` and `newMutableFlagWithSafeDefault` defaults, look for a
`BraveCachedFlag` override. Brave tests that `@DisableFeatures(X)` mark features
Brave never validated.

## Examples

- The enhanced send-tab-to-self sheet.
- M157: the sign-in button replaced the identity disc Brave no-ops.
- M157, 8094.1: `kYourSavedInfoSettingsPageAndroid` and
  `BraveMainPreferencesBase`.
