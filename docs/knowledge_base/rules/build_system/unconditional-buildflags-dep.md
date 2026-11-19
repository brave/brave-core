---
name: brave/unconditional-buildflags-dep
description:
  "When a GN target adds feature deps inside if (enable_feature), the feature's
  buildflags target must be an unconditional dep outside that block."
rubric: correctness
severity: blocking
---

**Rule.** When a target adds `deps +=` inside an `if (enable_feature)` block,
the feature's `buildflags` target MUST be in the target's deps outside the
conditional. NEVER put the `buildflags` dep only inside the guarded block.
Verify by running `gn check` with the buildflag disabled.

**Why.** The target's sources include the buildflags header for the
`#if BUILDFLAG(...)` check itself, which they need even when the feature is
disabled. A `buildflags` dep inside the conditional disappears in that
configuration, and the build or `gn check` fails there.

## Examples

Bad:

```gn
if (enable_brave_rewards) {
  deps += [
    "//brave/components/brave_rewards/common/buildflags",
    "//brave/components/brave_rewards/browser",
  ]
}
```

Good:

```gn
deps += [ "//brave/components/brave_rewards/common/buildflags" ]
if (enable_brave_rewards) {
  deps += [ "//brave/components/brave_rewards/browser" ]
}
```
