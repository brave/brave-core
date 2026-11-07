---
name: obsolete/deleted-files-brave-includes
rubric: obsolete
severity: regression
---

**Rule.** Brave doesn't include, depend on or name an upstream file the range
deleted or moved.

**Why.** Plain includes in platform-only sources break only that platform's
build, and a move into a restricted target breaks every Brave file that includes
it.

**How to check.** The candidates are the `file` rows of
`vpython3 tools/cr/codehealth/scripts/obsolete.py removed-names`: each non-Java
file the range deleted or moved (git detects renames, so `D` alone misses moves)
whose path Brave spells, with where. Java moves are sweep
[h](../android_ios/java-moves-break-xml-references.md)'s.

## Examples

- M157, 8101.1: `target_device_info.h` in iOS `brave_sendtab_api.mm`.
- M158, 8117.1: `profile_keyed_service_factory_ios.h` moved into a restricted
  `internal/` target, which every Brave iOS factory includes.
