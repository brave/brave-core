---
name: triage/deprecations-and-enforcement
rubric: triage
severity: deprecation
---

**Rule.** Subjects announcing a deprecation, a replacement idiom or an
enforcement get read for Brave code still on the old idiom.

**Why.** A campaign that no other sweep's mechanism covers still announces
itself in its subjects, often as a burst of commits that each convert one
directory, and it ends in enforcement that breaks Brave's remaining uses.

**How to check.** Group the candidates by the idiom they name (a burst of "Use X
form" commits is one campaign), and grep Brave's `chromium_src`, patches,
plasters and sources for the old idiom.

## Examples

- M158, 8104.1..8107.2: a mechanical "Use gin's compile-time
  `SetMethod<&T::M>()` form" campaign, found by skimming subjects.
