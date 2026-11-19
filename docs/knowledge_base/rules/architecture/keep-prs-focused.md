---
name: brave/keep-prs-focused
description:
  'A pull request has one main purpose; unrelated ride-along fixes, refactors,
  renames or reorderings belong in a separate pull request.'
rubric: change_scope
severity: should_fix
---

**Rule.** Flag changes unrelated to a pull request's main purpose: a fix for a
separate bug, refactoring unrelated to a behavioural change, large-scale
renaming or reformatting, or functions reordered for aesthetic reasons. They go
in a separate pull request. Small, closely related cleanups in code the change
already modifies are fine, as is formatting the formatter or presubmit requires,
in a commit of its own.

**Why.** Focused pull requests are easier to review and to revert, and keep the
history clear; mixed changes obscure each other's intent and make bisecting
regressions harder.

## Examples

Bad:

```
Add Foo toggle to settings
  + browser/ui/webui/settings/foo_handler.cc   (the feature)
  + components/bar/bar_service.cc              (unrelated crash fix)
  + components/baz/*.cc                         (functions reordered)
```

Good:

```
Add Foo toggle to settings
  + browser/ui/webui/settings/foo_handler.cc
  + browser/resources/settings/foo_page.ts
```
