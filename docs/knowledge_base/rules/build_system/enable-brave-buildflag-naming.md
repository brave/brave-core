---
name: brave/enable-brave-buildflag-naming
description:
  "A buildflag that must be told apart from Chromium's, or names a top-level
  Brave feature, is named enable_brave_<feature>."
rubric: readability
severity: should_fix
---

**Rule.** Name a new buildflag `enable_brave_<feature>` when it needs to be
distinguished from a Chromium flag or is a top-level Brave feature. Do not use
other shapes such as `brave_<feature>_enabled`. NEVER require the
`enable_brave_` prefix for a flag that is clearly Brave-specific by context,
such as one declared in a Brave component's own `buildflags.gni`
(`enable_tab_management_tool` in
`components/ai_chat/core/common/buildflags/buildflags.gni`).

**Why.** One naming shape makes Brave's top-level flags easy to find in
`args.gn` and keeps them from colliding with, or being mistaken for, Chromium's
`enable_<feature>` flags.

## Examples

Bad:

```gn
declare_args() {
  brave_perf_predictor_enabled = true
}
```

Good:

```gn
declare_args() {
  enable_brave_perf_predictor = true
}
```
