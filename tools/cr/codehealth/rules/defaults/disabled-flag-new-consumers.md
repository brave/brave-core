---
name: defaults/disabled-flag-new-consumers
rubric: defaults
severity: regression
---

**Rule.** New consumers of a feature Brave disables don't gate a security fix
behind it, and don't require its service.

**Why.** With the feature off in Brave, a fix upstream put behind it never runs,
and a consumer that `CHECK`s the disabled feature's service crashes.

**How to check.** The candidates add `IsEnabled(k…)` checks of features Brave
names. Keep those Brave disables, and read what the new branch guards.

## Examples

- The `AsBlob()` sensitive-entry check behind
  `kFileSystemAccessDirectoryIterationBlocklistCheck`.
- A miss, found in the rebase: `kPersonalContext` off made
  `DeviceInfoSyncClient` crash.
