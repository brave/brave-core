---
name: triage/moves-and-renames
rubric: triage
severity: regression
---

**Rule.** Moves, renames and splits of code Brave overrides or names leave no
Brave reference behind on the old name or path.

**Why.** A `chromium_src` override of a moved file stops applying silently, and
a renamed symbol in a `#define` override stops matching.

**How to check.** For each candidate, take the old paths and names it moves and
grep Brave's `chromium_src`, patches, plasters and sources for them. The overlap
sweep [a](../../SKILL.md#a-overlap-commits-touching-what-brave-overrides)
already reads commits that touch overridden files; this list catches the rest.
