---
name: triage/memory-safety-and-crashes
rubric: triage
severity: regression
---

**Rule.** Upstream fixes for use-after-frees, dangling pointers, races and
crashes get checked against Brave's copies and overrides of the fixed code.

**Why.** A Brave fork or full-function replacement of the fixed code keeps the
bug, and nothing tells Brave the fix happened.

**How to check.** For each candidate, read the files it fixes and check whether
Brave overrides or copies them (`chromium_src`, patches, plasters, or a fork
indexed by sweep [c](../../SKILL.md#c-security-hardening-and-forked-code)). If
so, check that the fix reaches Brave's version.
