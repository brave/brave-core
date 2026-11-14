---
name: brave/feature-checks-over-platform-checks
description:
  'Behaviour that depends on a feature or capability is conditioned on a check
  of that feature, not on the platform.'
rubric: design
severity: should_fix
---

**Rule.** When code differs because a feature or capability is present or
absent, test that (a feature flag, a pref, a capability query), not the platform
it currently happens to correlate with. Keep `#if BUILDFLAG(IS_*)` for code that
really depends on the platform.

**Why.** A platform check hard-codes today's correlation: when the feature
reaches another platform, or is turned off on this one, the platform check gives
the wrong answer.

## Examples

Bad:

```cpp
#if BUILDFLAG(IS_ANDROID)
  // Don't show notifications.
#endif
```

Good:

```cpp
if (IsDoNotDisturbEnabled()) {
  // Don't show notifications.
}
```
