---
name: brave/group-files-by-component-not-platform
description:
  "New files go in their feature's directory, with a platform subdirectory
  (brave_rewards/android/), not in a feature subdirectory of a platform
  directory (android/rewards/)."
rubric: readability
severity: nit
---

**Rule.** Put a feature's platform-specific files in a platform subdirectory of
the feature's directory, such as `browser/brave_rewards/android/`. Do not add a
feature subdirectory to a platform directory, such as
`browser/android/rewards/`.

**Why.** Grouping by component keeps a feature's code together, and matches
Chromium's layout (`chrome/browser/history/android/`).

## Examples

Bad:

```
brave/browser/android/rewards/brave_rewards_native_worker.cc
```

Good:

```
brave/browser/brave_rewards/android/brave_rewards_native_worker.cc
```
