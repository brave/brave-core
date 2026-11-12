---
name: overlap/parallel-implementations
rubric: overlap
severity: regression
---

**Rule.** Brave pins every flag that a factory reads to pick between a UI Brave
customizes and a parallel rebuild of it.

**Why.** The new path constructs the plain upstream class in files Brave doesn't
hook. A per-type migration flag works the same way.

**How to check.** A burst of commits under a new directory that rebuilds a UI
Brave customizes means a factory will pick between the two. The candidates add a
feature check in files Brave overrides; find the flag and check Brave pins it.
Grep `IsInfoBarMigrated(` in files Brave overrides.

## Examples

- ActionAppMenu, the WebUI toolbar and omnibox.
- M157, 8097.1: `kCentralizedInfoBarFramework` skips Brave's infobar delegate
  renames.
