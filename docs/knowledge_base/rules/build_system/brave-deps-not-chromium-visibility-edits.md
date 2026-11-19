---
name: brave/brave-deps-not-chromium-visibility-edits
description:
  'To reach Brave code from a Chromium target, depend on a high-level //brave/
  target that already has visibility, instead of adding Brave targets to a
  Chromium visibility list.'
rubric: design
severity: should_fix
---

**Rule.** When a Chromium target (through a patch, a plaster or a `chromium_src`
GN override) needs Brave code, add a dep on a high-level `//brave/` target that
already has visibility, such as `//brave/utility` or `//brave/browser`. Do not
add `//brave/...` entries to the `visibility` of a Chromium target.

**Why.** Visibility lists exist to prevent exactly this kind of cross-boundary
dependency. Widening one lets a low-level Chromium target reach into Brave
internals, and every such edit is another patch to rebase.

## Examples

Bad:

```gn
visibility += [ "//brave/components/brave_rewards/browser" ]
```

Good:

```gn
deps += [ "//brave/utility" ]
```
