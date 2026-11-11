---
name: security/build-flag-splits
rubric: security
severity: regression
---

**Rule.** Brave code guarded by an upstream `BUILDFLAG` or GN arg still gets
what it relied on after upstream splits the flag.

**Why.** A split leaves the old flag meaning less than before, and upstream's
TODOs often announce a new default for the split-off half.

**How to check.** For each candidate commit, grep Brave for the old `BUILDFLAG`
and GN arg, and read upstream's TODOs about future defaults.

## Examples

- Key pinning split from the HSTS preload list; Brave iOS relies on the pins.
