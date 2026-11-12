---
name: enforcement/scripted-cleanups
rubric: enforcement
severity: deprecation
---

**Rule.** Brave doesn't use the enum values that upstream's clean-up scripts
announce and remove.

**Why.** Scripted clean-ups delete old values across the tree, and Brave's uses
break at the next lift.

**How to check.** For each candidate commit (a clean-up script, or a removed
`NotFatalUntil` value), grep Brave for the values it removes, e.g.
`git grep -n 'NotFatalUntil::M1[0-4][0-9]'`.

## Examples

- `clean-up-not-fatal-until.py` deletes old `NotFatalUntil` values.
