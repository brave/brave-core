---
name: enforcement/warning-opt-outs
rubric: enforcement
severity: deprecation
---

**Rule.** Brave code builds in directories where upstream enables a warning, and
Brave's suppressions of it are tracked as debt.

**Why.** Per-directory enablement shows up as a removed opt-out in a `BUILD.gn`,
and Brave's sources compiled in that target then hit the warning.

**How to check.** The candidates remove an opt-out, as in
`git log -G'no_exit_time_destructors' $A..$B -- '*.gn'`. For each directory,
check Brave's sources compiled there (`chromium_src` overrides, Brave files
listed in the target) for the warning. Brave's remaining debt is its
suppressions (`[[clang::no_destroy]]`).
