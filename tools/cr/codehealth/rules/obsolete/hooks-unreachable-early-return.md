---
name: obsolete/hooks-unreachable-early-return
rubric: obsolete
severity: obsolete
---

**Rule.** A Brave hook placed after an early return upstream added is still
reached, or goes away.

**Why.** "Disable X if flag is off" commits return before code Brave hooked
into, and Brave's hook then never runs, with no error.

**How to check.** The candidates add an early return behind a feature or flag
check to a file Brave overrides, or say so in their subject. For each, check
whether Brave's hook in the file sits after the new return, and whether Brave's
feature state takes it.

## Examples

- The offer-notification promo-code exclusion.
