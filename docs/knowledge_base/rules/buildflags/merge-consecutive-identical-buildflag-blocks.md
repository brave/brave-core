---
name: brave/merge-consecutive-identical-buildflag-blocks
description:
  'Consecutive code regions guarded by the same #if BUILDFLAG(...) condition are
  merged into one guard block.'
rubric: readability
severity: nit
---

**Rule.** When a change leaves two or more consecutive regions guarded by the
same `#if BUILDFLAG(...)` condition, with nothing but blank lines between them,
merge them into a single `#if` ... `#endif` block. Merging a guarded include
into a following code block is limited by include ordering: see
`brave/keep-guarded-includes-with-includes`.

**Why.** Back-to-back identical guards are redundant: they add `#if`/`#endif`
pairs to read and keep in sync, and make the guarded region look like several
unrelated ones.

## Examples

Bad:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void RegisterRewardsPrefs(PrefRegistrySimple* registry);
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void MigrateRewardsPrefs(PrefService* prefs);
#endif
```

Good:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
void RegisterRewardsPrefs(PrefRegistrySimple* registry);
void MigrateRewardsPrefs(PrefService* prefs);
#endif
```
