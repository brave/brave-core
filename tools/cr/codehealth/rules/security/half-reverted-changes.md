---
name: security/half-reverted-changes
rubric: security
severity: regression
---

**Rule.** When Brave reverts an upstream change, it reverts all of it.

**Why.** The change may have a Java or build half that Brave didn't revert, and
that half keeps the behaviour Brave meant to undo.

**How to check.** For each upstream change Brave reverts (patches and plasters
that undo a commit, Brave commits titled "Revert"), read the upstream commit's
whole `--stat` and check each half.

## Examples

- The manifest version change also changed `VersionInfo.getProductVersion()` on
  Android.
