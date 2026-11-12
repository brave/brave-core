---
name: obsolete/workarounds-upstream-removed
rubric: obsolete
severity: obsolete
---

**Rule.** When upstream removes a workaround from its own code, Brave's copy of
it, or Brave's own workaround for the same symptom, goes too.

**Why.** Brave copied the workaround, or added one for the same symptom, and
nothing tells it the cause is gone.

**How to check.** The candidates are commits whose subject or body says they
remove a workaround, and commits that remove workaround or `TODO(crbug…)`
comments from files Brave overrides. For each, grep Brave for the workaround's
names and symptom.

## Examples

- `ViewShadow::OnLayerRecreated`.
- Single-process `ScopedChromeExtensionsClient` sharing.
