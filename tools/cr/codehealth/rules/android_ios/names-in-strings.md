---
name: android_ios/names-in-strings
rubric: android_ios
severity: regression
---

**Rule.** Every name Brave hardcodes as a string in `BraveReflectionUtil` calls
and `BytecodeTest` assertions still names a member upstream declares at the new
tag, with the same parameter list.

**Why.** No compiler checks these names, and none fails loudly:
`BraveReflectionUtil` returns null in release builds, and `BytecodeTest` is an
on-device test a Linux build never runs. A rename or a new overload silently
switches the Brave behaviour off.

**How to check.** The candidate list is the output of
`tools/cr/codehealth/scripts/java_names.py` for the range. File an item per
`broke` row, and per `debt` row not tracked yet. Read `review` rows by hand:
they match only through a type variable, or a class the test computes. A
`reviewed` row is the same in a class the range didn't change, so it's only
counted. Bytecode adapter targets aren't covered yet
([#60348](https://github.com/brave/brave-browser/issues/60348)); see
`bytecode-adapters`.

## Examples

- M157, 8093.0: `TabCollectionTabModelImpl.isTabModelRestored()` was renamed,
  which turned Brave's tab auto-grouping off in release builds.
- M158: `SingleWebsiteSettings.setupContentSettingsPreference` gained a
  `PermissionSetting`, emptying two per-site rows; the tool reports it as "arity
  differs" at 157.
- M158, 8107.2: `UndoBarController.showUndoBar` kept its name, not its
  signature.
