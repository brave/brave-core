---
name: modularize-brave-browser
description:
  'Modularize Brave browser code: move a //brave/browser directory out of the
  brave_chrome_browser_sources monolith into its own headers/impl/test targets,
  or remove an allow_circular_includes_from entry from Brave GN files. Triggers
  on: modularize, componentize, Project Bedrock, split into header/impl targets,
  move out of sources.gni, remove circular includes,
  allow_circular_includes_from, break circular dependency,
  circular_includes_brave.gni.'
argument-hint: '[directory | target ...]'
---

# Modularize Brave browser code

Two workflows for the same goal, small GN targets with no circular includes:

- **[A] Modularize a directory**: move its files out of
  `brave_chrome_browser_sources` (or `brave/test/BUILD.gn`) into its own
  `BUILD.gn`.
- **[B] Remove an `allow_circular_includes_from` entry.**

They feed each other. [B] often finds that the included header is in the
monolith, which needs [A]; [A] often leaves an entry dead, which [B] removes.

Rules are in `docs/best-practices/modularization.md` (MOD-\*); plan and tracking
are in https://github.com/brave/brave-browser/issues/59199. Upstream's
equivalents are
[`//agents/projects/bedrock/modularize-chrome-browser`](https://chromium.googlesource.com/chromium/src/+/main/agents/projects/bedrock/modularize-chrome-browser/SKILL.md)
and
[`//agents/skills/remove-circular-includes`](https://chromium.googlesource.com/chromium/src/+/main/agents/skills/remove-circular-includes/SKILL.md).

Do not commit, branch or push unless asked. When asked to plan, produce a
numbered list of steps only.

## Where things live

| What                                                                           | Where                                                                                                                                                                                                        |
| ------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Monolith sources                                                               | `brave_chrome_browser_sources` in `browser/sources.gni`, extended by `brave_browser_<dir>_sources` in `browser/<dir>/sources.gni`; compiled into `//brave/browser:core`, a dep of `//chrome/browser:browser` |
| Deps of the monolith                                                           | `brave_chrome_browser_deps` (and `brave_browser_<dir>_deps`)                                                                                                                                                 |
| Monolith tests                                                                 | `sources` of `brave_unit_tests` and `brave_browser_tests` in `test/BUILD.gn`; modular tests are deps of those targets                                                                                        |
| `brave_chrome_browser_allow_circular_includes_from` (`browser/sources.gni`)    | declared on `//brave/browser:core`                                                                                                                                                                           |
| `brave_ui_allow_circular_includes_from` (`browser/ui/config.gni`)              | declared on `//brave/browser/ui:ui`, and on `//chrome/browser/ui:ui` through the next list                                                                                                                   |
| `brave_chrome_browser_ui_allow_circular_includes_from` (`browser/sources.gni`) | declared on `//chrome/browser/ui:ui`, via `rewrite/chrome/browser/ui/BUILD.gn.yaml`                                                                                                                          |
| `[ ":brave_prefs_util_impl" ]` (`browser/extensions/BUILD.gn`)                 | declared on `//brave/browser/extensions:extensions`                                                                                                                                                          |
| `build/circular_includes_brave.gni`                                            | the Brave targets allowed to declare the key ([`//.gn`](https://chromium.googlesource.com/chromium/src/+/main/.gn) fails `gn gen` for any other); it does not list entries                                   |

An entry in `brave_ui_allow_circular_includes_from` applies to two targets, so
removing it must pass `gn check` for both.

## Terms

`A`, `B` and "includes" are as defined in
[MOD-004](../../../docs/best-practices/modularization.md#MOD-004).

## Setup

Use output directories for each platform the files build on: Linux, plus
Android, Mac and Windows when the files or their guards are platform-specific.
Run `gn gen out/<dir>` on each until it passes. For [B], capture a baseline per
directory before any edit:

```sh
siso query graph -C out/<dir> > out/<dir>/baseline.jsonl
```

## [A] Modularize a directory

### 1. Inventory

A `BUILD.gn` in the directory does not mean it is modularized. Check whether its
files are still in a monolith:

```sh
grep -n "browser/<dir>/" browser/sources.gni browser/*/sources.gni test/BUILD.gn
```

Categorize every file:

| Files                               | Target                                                         |
| ----------------------------------- | -------------------------------------------------------------- |
| `.h` that other modules include     | `:<dir>` (headers)                                             |
| `.cc` and headers only they include | `:impl`                                                        |
| `*_unittest.cc`                     | `:unit_tests`                                                  |
| `*_browsertest.cc`                  | `:browser_tests`                                               |
| `*_interactive_uitest.cc`           | leave in `test/BUILD.gn` unless a target for it already exists |

Read each header to see which deps its declarations need, as opposed to deps
only its `.cc` needs. Find who includes the headers, including `chromium_src`
overrides and plaster rewrites:

```sh
git grep -n '#include "brave/browser/<dir>/' -- '*.cc' '*.h' '*.mm'
```

If a file includes a header that is still in a monolith (another Brave directory
in `brave_chrome_browser_sources`, `//chrome/browser:core`,
`//chrome/browser/ui:ui`, `//brave/test:*` sources), it cannot move yet
(MOD-005). Leave it, and record the blocking header for the tracking issue, or
modularize the other directory first.

Keep a CL to about 30 files. Split a large directory into several CLs.

### 2. Write the `BUILD.gn`

Use the header/impl split (MOD-003). Use a single target with `public`/`sources`
only for a small module with few consumers.

```gn
# Copyright (c) <year> The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

source_set("<dir>") {
  sources = [
    "foo.h",
    "foo_factory.h",
  ]
  # Only deps whose types appear in these headers.
  public_deps = [ "//components/keyed_service/core" ]
}

source_set("impl") {
  sources = [
    "foo.cc",
    "foo_factory.cc",
  ]
  deps = [
    ":<dir>",
    "//chrome/browser/profiles:profile",
  ]
}

source_set("unit_tests") {
  testonly = true
  sources = [ "foo_unittest.cc" ]
  deps = [
    ":<dir>",
    "//testing/gtest",
  ]
}

source_set("browser_tests") {
  testonly = true
  defines = [ "HAS_OUT_OF_PROC_TEST_RUNNER" ]
  sources = [ "foo_browsertest.cc" ]
  deps = [
    ":<dir>",
    "//chrome/test:test_support_ui",
  ]
}
```

Follow [MOD-003](../../../docs/best-practices/modularization.md#MOD-003) for
deps and guards, and
[MOD-006](../../../docs/best-practices/modularization.md#MOD-006) for factories.
In addition:

- Omit targets with no files.
- Give `:impl` a `visibility` that admits only its linking targets when
  possible.

### 3. Update the parents

- Remove the files from `brave_chrome_browser_sources` or
  `brave_browser_<dir>_sources`. If a per-directory `sources.gni` ends up empty,
  delete it, its `import` and its `+=` lines in `browser/sources.gni`.
- Add `//brave/browser/<dir>:impl` to `brave_chrome_browser_deps` so it links
  into `//brave/browser:core`.
- Move deps only the moved files needed from the `brave_browser_<dir>_deps` list
  into the new targets.
- Remove the tests from `brave_unit_tests` / `brave_browser_tests` sources in
  `test/BUILD.gn` and add `//brave/browser/<dir>:unit_tests` / `:browser_tests`
  to their deps.
- A `chromium_src` override or plaster rewrite that includes a moved header
  needs its upstream target to depend on `:<dir>` (MOD-007), through the
  matching `sources.gni` deps list or a `rewrite/` substitution.

### 4. Fix `gn check` errors

- "Include not allowed" or "can't include this header from here": add the
  missing dep to the failing target, or promote it to `public_deps` on the
  target owning the header if its types appear in that target's headers.
- An error that would need a new `allow_circular_includes_from` entry means the
  file is blocked (step 1). Do not add the entry (MOD-001).
- Once the files are out, check the circular lists for entries that existed only
  for them and remove them with [B].

## [B] Remove an `allow_circular_includes_from` entry

1. Remove the entry from the list. If the declaring target has no entries left,
   remove the key and remove the target from `circular_includes_brave.gni`.
2. Run `gn gen out/<dir>` and `gn check out/<dir>` on every directory.
3. Find the first matching row of the
   [MOD-004](../../../docs/best-practices/modularization.md#MOD-004) table,
   then:

   - **Dead entry**: run `gn check --fix`.
   - **Header in a monolith**: for a Brave header, run [A] on its directory and
     retry. For an upstream header, restore the entry, keep or add a comment
     naming the header, and report it as blocked for the tracking issue.
   - **Real cycle**: fix the design first
     ([MOD-006](../../../docs/best-practices/modularization.md#MOD-006)).
   - **`component`/`static_library` with sources**: copy `A` into
     `source_set("A_sources")` (or `static_library("A_sources")`), turn `A` into
     a `group` (a `static_library` without sources and configs if it must stay
     one), add `A_sources` to `A`'s `public_deps`, remove `deps` and
     `public_deps` from `A_sources`, then let `gn check --fix` add the deps
     `A_sources` needs and add `A_sources` to `B`.
   - **Backwards arrow**: run `gn check --fix` so `B` depends on `A`, run
     `gn gen`, then delete the edge in the reported loop that has no matching
     `#include`.

4. If `gn check --fix` reports a visibility error, fix the visibility or choose
   the correct target.

## Verify

- `gn check` passes on every directory.
- [B] with metadata-only changes: `siso query graph` after the edit matches the
  baseline (only `A`→`A_sources` object renames are expected when splitting).
- Build with
  `pnpm run build --ignore_compile_failure --target=brave:all --gn=enable_updater:true --gn=use_prebuilt_omaha4:false`,
  even when `gn check` passes.
- [A]: run the moved tests, e.g.
  `pnpm run test brave_unit_tests --filter="Foo*"` and
  `pnpm run test brave_browser_tests --filter="Foo*"`, and confirm the same
  number of tests runs as before the move.

## Rules

[MOD-001](../../../docs/best-practices/modularization.md#MOD-001),
[MOD-005](../../../docs/best-practices/modularization.md#MOD-005) and
[MOD-008](../../../docs/best-practices/modularization.md#MOD-008) apply to every
change.

## Commit message (when asked to commit)

Follow the `cl-description` conventions, with the footer
`Bug: https://github.com/brave/brave-browser/issues/59199`.

- [A] Subject: `Modularize //brave/browser/<dir>`. Body: which targets now exist
  and what left the monolith.
- [B] Subject: `Remove allow_circular_includes_from from //path:target`. Body:
  why the entry existed, then how it was removed.
