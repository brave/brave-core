---
name: brave/no-compound-buildflags
description:
  'A buildflag must not combine channel, platform and official_build conditions;
  use separate, independently testable flags.'
rubric: testing
severity: should_fix
---

**Rule.** Do not define a buildflag whose value combines the release channel
(`brave_channel`), the platform and `is_official_build`, such as one enabled
only on nightly, desktop, official builds. Use separate flags that can each be
set and tested on their own.

**Why.** Compound flags break during branch migration (nightly to beta to
release), and they create configurations that are nearly impossible to reproduce
and test locally.

## Examples

Bad:

```gn
declare_args() {
  enable_brave_foo =
      brave_channel == "nightly" && !is_android && !is_ios && is_official_build
}
```

Good:

```gn
declare_args() {
  enable_brave_foo = !is_android && !is_ios
}
```
