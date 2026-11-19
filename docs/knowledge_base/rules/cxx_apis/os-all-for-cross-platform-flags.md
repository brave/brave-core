---
name: brave/os-all-for-cross-platform-flags
description:
  'A feature flag entry in about_flags that is available on every platform uses
  kOsAll, not a list of platform constants.'
rubric: correctness
severity: should_fix
---

**Rule.** When a flag registered in `about_flags.cc` (or `about_flags.mm`) is
meant for all platforms, its platform mask is `kOsAll`. Do not combine constants
such as `kOsDesktop | kOsAndroid` to mean "everywhere". Listing platforms is
right only when the flag is deliberately limited to them.

**Why.** A hand-written list of platforms reads as a deliberate restriction,
which readers have to decode against the platform constants, and silently leaves
out any platform it forgets. `kOsAll` says what is meant.

## Examples

Bad:

```cpp
{"brave-my-feature", ..., kOsDesktop | kOsAndroid}
```

Good:

```cpp
{"brave-my-feature", ..., kOsAll}
```
