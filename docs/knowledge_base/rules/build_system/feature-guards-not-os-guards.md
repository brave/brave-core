---
name: brave/feature-guards-not-os-guards
description:
  "Feature-specific GN sources and deps, and feature-specific C++ code, are
  guarded by the feature's own buildflag, never by OS guards approximating where
  the feature is built."
rubric: correctness
severity: should_fix
---

**Rule.** When code exists only because a feature is built, guard it with that
feature's flag: `if (enable_tor)` in GN, `#if BUILDFLAG(ENABLE_TOR)` in C++.
NEVER use an OS guard (`is_linux`, `is_win`, `!is_android && !is_ios`,
`#if !BUILDFLAG(IS_ANDROID)`) as a stand-in for "the platforms this feature is
on". Platform guards without buildflags are deprecated for feature code.

**Why.** The platforms a feature is enabled on are decided by its buildflag. An
OS guard copies that decision by hand, and gets out of sync with the actual
feature flag logic as soon as the flag's definition changes, leaving code built
where the feature is off, or missing where it is on.

## Examples

Bad:

```gn
if (!is_android && !is_ios) {
  sources += [ "brave_wallet_utils.cc" ]
}
```

Good:

```gn
if (brave_wallet_enabled) {
  sources += [ "brave_wallet_utils.cc" ]
}
```
