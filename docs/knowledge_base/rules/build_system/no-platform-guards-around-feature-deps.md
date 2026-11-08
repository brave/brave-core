---
name: brave/no-platform-guards-around-feature-deps
description:
  'Within a GN target, feature deps are added behind their buildflag only, never
  nested inside a platform guard, so the GN guard matches the C++ one.'
rubric: correctness
severity: should_fix
---

**Rule.** Add a feature's deps under that feature's buildflag alone
(`if (enable_tor)`). Do not nest the buildflag block inside a platform guard
(`if (is_mac) { if (enable_tor) { ... } }`). The condition guarding a dep in GN
must be the one guarding its use in C++ (`#if BUILDFLAG(ENABLE_TOR)`).

**Why.** When the GN guard is narrower than the C++ one, the code compiles on a
platform where its deps are missing, and the build breaks there as soon as the
buildflag is enabled on it.

## Examples

Bad:

```gn
if (is_mac) {
  if (enable_tor) {
    deps += [ "//brave/components/tor" ]
  }
}
```

Good:

```gn
if (enable_tor) {
  deps += [ "//brave/components/tor" ]
}
```
