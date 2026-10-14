---
name: android_ios/removed-flag-constants
rubric: android_ios
severity: obsolete
---

**Rule.** Brave's `@EnableFeatures` and `@DisableFeatures` don't name
`ChromeFeatureList` constants upstream removed in the range.

**Why.** A removed constant fails only the Android test build, and a test that
toggles a feature that no longer exists tests nothing.

**How to check.** Take the constants the candidate commits remove from
`ChromeFeatureList`, and grep Brave's Java for them.

## Examples

- M157, 8097.1: `SETTINGS_SINGLE_ACTIVITY`.
