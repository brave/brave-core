---
name: android_ios/guard-moved-out-of-shared-helper
rubric: android_ios
severity: regression
---

**Rule.** When upstream moves an SDK or platform check out of a shared helper,
Brave's callers of the helper make the check themselves.

**Why.** Brave entry points that relied on the helper's guard now run where the
API isn't available, which crashes only on old Android versions.

**How to check.** The candidate commits remove `SDK_INT`, `VERSION_CODES` or
`isAndroid*` checks, or a Javadoc "callers must check". Grep Brave for callers
of the helper each one changed.

## Examples

- M157, 8093.0: `PictureInPicture.isEnabled()` lost its Android R check, and
  Brave's YouTube PiP entry points relied on it.
