---
name: remove-circular-includes
description:
  'Remove an allow_circular_includes_from entry from Brave GN files while
  keeping the build graph unchanged and gn check clean on every platform.
  Triggers on: remove circular includes, allow_circular_includes_from, break
  circular dependency, circular_includes_brave.gni.'
argument-hint: '[target ...]'
---

# Remove `allow_circular_includes_from`

Remove entries from Brave's circular-include lists one target at a time. Rules
are in `docs/best-practices/modularization.md`; the plan and tracking are in
https://github.com/brave/brave-browser/issues/59199. Upstream's equivalent is
`//agents/skills/remove-circular-includes` (crbug.com/556482973).

Do not commit, branch or push unless asked.

## Where the entries live

| List                                                                           | Declared on                                                                 |
| ------------------------------------------------------------------------------ | --------------------------------------------------------------------------- |
| `brave_chrome_browser_allow_circular_includes_from` (`browser/sources.gni`)    | `//brave/browser:core` (holds `brave_chrome_browser_sources`)               |
| `brave_ui_allow_circular_includes_from` (`browser/ui/config.gni`)              | `//brave/browser/ui:ui`, and `//chrome/browser/ui:ui` through the next list |
| `brave_chrome_browser_ui_allow_circular_includes_from` (`browser/sources.gni`) | `//chrome/browser/ui:ui`, via `rewrite/chrome/browser/ui/BUILD.gn.yaml`     |
| `[ ":brave_prefs_util_impl" ]` (`browser/extensions/BUILD.gn`)                 | `//brave/browser/extensions:extensions`                                     |

An entry in `brave_ui_allow_circular_includes_from` applies to two targets, so
removing it must pass `gn check` for both.

`build/circular_includes_brave.gni` lists the Brave targets allowed to declare
the key (`//.gn` fails `gn gen` for any other). It does not list entries.

## Terms

Target `A` includes target `B` when `A`'s sources `#include` `B`'s public
headers (or `sources` with `friend`). `A` should depend on `B` exactly then.
`allow_circular_includes_from = [ B ]` on `A` requires `B` to be in `A`'s deps
and lets `B` include `A`'s headers, against the dependency direction.

## Setup

Use output directories for each platform the target's files build on (Linux plus
Android, Mac and Windows when the target has platform-specific files or guards).
Before any edit, capture a baseline per directory:

```sh
gn gen out/<dir>
siso query graph -C out/<dir> > out/<dir>/baseline.jsonl
```

## Diagnose

1. Remove the entry from the list. If the declaring target has no entries left,
   remove the key and remove the target from `circular_includes_brave.gni`.
2. Run `gn gen out/<dir>` and `gn check out/<dir>` on every directory.
3. Apply the first case that matches:

   - `A` does not transitively depend on `B`, or `B` does not include `A`: the
     entry is dead. Run `gn check --fix`.
   - `B` includes a header that is in `A` only because `A` is a monolith
     (`brave_chrome_browser_sources`, `//chrome/browser:browser`,
     `//chrome/browser/ui:ui`). If it is a Brave header, move it into its own
     headers target with an `:impl` (MOD-003), then retry. If it is an upstream
     header, restore the entry, keep or add the comment naming the header, and
     report it as blocked for the tracking issue.
   - `A` includes `B` and `B` includes `A`: real cycle. Extract the shared
     declarations into a third headers target, or merge the targets. Prefer
     fixing the design (constructor injection, `UnownedUserData`, dependency
     inversion) over moving files.
   - `A` is a `static_library`/`component` with `public`/`sources`: copy it into
     `source_set("A_sources")`, turn `A` into a `group`/`static_library` without
     sources, and add `A_sources` to its `public_deps`. Let `gn check --fix` add
     deps to `A_sources`.
   - Otherwise `B` includes `A`, and `A` depends on `B` without including it:
     run `gn check --fix` so `B` depends on `A`, run `gn gen`, then delete the
     edge in the reported loop that has no matching `#include`.

4. If `gn check --fix` reports a visibility error, fix the visibility or choose
   the correct target. Do not add `check_includes = false` or `// nogncheck`
   (the only accepted form is `// nogncheck crbug.com/40147906` for conditional
   includes GN cannot model).

## Verify

- `gn check` passes on every directory.
- For a pure metadata change, `siso query graph` after the edit matches the
  baseline (only `A`→`A_sources` object renames are expected when splitting).
- Build the touched targets with
  `pnpm run build --ignore_compile_failure --target=brave:all --gn=enable_updater:true --gn=use_prebuilt_omaha4:false`
  for changes that move sources or demote `public_deps`; those can pass
  `gn check` and fail at link time.
- Platform-only consumers (`.mm`, `is_win`, `is_android`) are invisible to a
  Linux check; verify them explicitly.

## Banned

- `-=` on `sources`; one file listed in two targets.
- Platform-specific header sets unless a platform header forces the split.
- Unrelated refactoring in the same change.

## Commit message (when asked to commit)

Follow the `cl-description` conventions. Subject:
`Remove allow_circular_includes_from from //path:target`. Body: why the entry
existed, then how it was removed. Footer:
`Bug: https://github.com/brave/brave-browser/issues/59199`.
