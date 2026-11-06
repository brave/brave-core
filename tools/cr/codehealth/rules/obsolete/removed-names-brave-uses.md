---
name: obsolete/removed-names-brave-uses
rubric: obsolete
severity: regression
---

**Rule.** Brave doesn't use a name the range removed from upstream.

**Why.** A Linux build only proves the names its own sources use: Android, iOS,
Mac and Windows sources break on removed names only on that platform's build.

**How to check.** The candidates are the `name` rows of
`vpython3 tools/cr/codehealth/scripts/obsolete.py removed-names`: type,
function, `k` constant and `IDS_` message names on `-` lines of the range's code
diff and on no `+` line, that Brave's sources (outside patches, plasters and
vendored code) spell, and that the tree at the new tag no longer has. Each comes
with the upstream files that had it and where Brave uses it. A Brave name that
only shares the spelling shows by being elsewhere, and a message Brave's own grd
defines by pointing at it: drop them. `comment` rows are names Brave only
mentions in comments: not breaks, but a stale comment is a small "Dead files and
stale references" item.

## Examples

- M159, 8153.0: `IDS_PROMOTE_PREFLIGHT_*_ERROR` and `IDS_UPGRADE_ERROR_DETAILS`,
  deleted strings Brave's Mac-only Keystone code uses.
- M159: the iOS CSV importer break, and the removed
  `android_override_version_name` GN arg, which GN only reports as a "Build
  argument has no effect" warning.
- M158, 8115.1: Android and iOS build breaks such as `OriginalProfileSupplier`
  and `CSPDirectiveName::TreatAsPublicAddress`.
- M158, 8127.2: `GetAllSyncedClientsGroupedByName`, left in a comment in
  `browsing_history_cleaner.cc`.
- Not a use, M159, 8144.0: `SetUpHTTPSServer`, an iOS free function upstream
  removed, and the playlist browser test's own virtual.
