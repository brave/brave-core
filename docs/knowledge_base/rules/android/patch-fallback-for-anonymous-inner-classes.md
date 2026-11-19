---
name: brave/patch-fallback-for-anonymous-inner-classes
description:
  A .patch of upstream Java code is accepted when Brave's override system cannot
  reach an anonymous inner class, and the PR documents why and links the
  tracking issue.
rubric: change_scope
severity: should_fix
---

**Rule.** When the change to upstream Chromium Java code is inside an anonymous
inner class, which Brave's override system (bytecode adapters, Brave subclasses)
cannot handle, a `.java.patch` file is the accepted fallback: do not ask for an
override instead. The PR MUST document that reason and link to the tracking
issue for better override support.

**Why.** Anonymous inner classes have no name an adapter or subclass can target,
so a patch is the only mechanism that reaches them. Recording why keeps the
patch from being mistaken for avoidable, and the issue link tracks replacing it
once the override system can.

## Examples

Bad:

```
PR description: "Patch TabGridDialogMediator.java to hide the share button."
```

Good:

```
PR description: "The share button's listener is an anonymous inner class, which
the bytecode adapters cannot override, so this uses a .java.patch. Tracking
better override support: https://github.com/brave/brave-browser/issues/NNNNN"
```
