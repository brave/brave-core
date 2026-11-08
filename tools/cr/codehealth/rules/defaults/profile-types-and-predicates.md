---
name: defaults/profile-types-and-predicates
rubric: defaults
severity: regression
---

**Rule.** New profile kinds, and changed meanings of profile predicates, don't
bypass Brave's private-window checks, and Brave's own predicate overrides keep
upstream's invariants.

**Why.** Brave's checks are written against today's predicates. Brave makes
`Profile::IsPrimaryOTRProfile()` true for Tor, which breaks code written for one
primary OTR profile.

**How to check.** The candidates add `IsPrimaryOTRProfile`,
`IsIncognitoProfile`, `GetPrimaryOTRProfile` and `IsOffTheRecord` consumers, or
change profile types. Read each against Tor and private windows, and grep
Brave's `IsIncognitoProfile()`, `IsOffTheRecord()` and
`BrowserProfileType::kIncognito` call sites for changed meanings.

## Examples

- Enterprise isolated mode is no longer `IsIncognitoProfile()`.
- A miss, found in the rebase: Brave's Tor `IsPrimaryOTRProfile()` broke
  upstream's cookies API tracker, written for one primary OTR profile.
