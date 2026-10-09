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
disable-model-invocation: true
---

# Modularize Brave browser code

Two workflows for the same goal, small GN targets with no circular includes:

- **[A] Modularize a directory**: move its files out of
  `brave_chrome_browser_sources` (or `brave/test/BUILD.gn`) into its own
  `BUILD.gn`.
- **[B] Remove an `allow_circular_includes_from` entry.**

They feed each other. [B] often finds that the included header is in the
monolith, which needs [A]; [A] often leaves an entry dead, which [B] removes.

Do not commit, branch or push unless asked. When asked to plan, produce a
numbered list of steps only.

## Rules

These apply to every change made with this skill.

- **No new circular includes.** Never add an entry to any
  `allow_circular_includes_from` list, and never set the key on a new target
  (including an `:impl` listing `//chrome/browser:browser_public_dependencies`).
  `gn gen` fails only for a new declaring target; a new entry in an existing
  list builds cleanly, so it must not be added in the first place. If a file can
  only reach a header through a cycle, leave the file where it is or remove the
  cycle.
- **A file blocked by a monolith stays put.** If a file includes a header still
  in a monolith (another directory in `brave_chrome_browser_sources`,
  `//chrome/browser:core`, `//chrome/browser/ui:ui`, `//brave/test:*` sources),
  it cannot move until that header's owner is modularized. Report the blocking
  header to the user.
- **No suppressions.** Never use `check_includes = false`. Never use
  `// nogncheck`, except exactly `// nogncheck crbug.com/40147906` for a
  conditional include GN cannot model.
- **Explicit sources.** Never use `-=` on `sources`, and never list one file in
  two targets.
- **Direct deps.** Never depend on
  `//chrome/browser:browser_public_dependencies`; upstream has removed it. List
  the deps the files actually use.
- **Same targets on every platform.** Keep headers targets identical across
  platforms unless a platform-only header forces a split.
- **Small, mechanical changes.** One directory or one entry (or a tightly
  related group) per CL, about 30 files at most. Do not mix a move with a
  behaviour change or unrelated cleanup.
- **Platform-only code is invisible to a Linux check.** Files such as `.mm`,
  `*_win.cc` and `*_android.cc`, and anything under `if (is_win)` or
  `if (is_android)`, must be checked on their own platform.

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
| `build/circular_includes_brave.gni`                                            | the Brave targets allowed to declare the key (`//.gn` fails `gn gen` for any other); it does not list entries                                                                                                |

An entry in `brave_ui_allow_circular_includes_from` applies to two targets, so
removing it must pass `gn check` for both.

## Terms

Target `A` _includes_ target `B` when `A`'s sources `#include` `B`'s public
headers (or `sources`, with `friend`). `A` should depend on `B` exactly then.

`allow_circular_includes_from = [ B ]` on target `A` requires `B` to be in `A`'s
deps, and lets `B` include `A`'s headers against the dependency direction.

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

Leave any file blocked by a monolith header (see Rules), or modularize the other
directory first. Split a directory with more than about 30 files into several
CLs.

### 2. Write the `BUILD.gn`

Split headers from implementation. Consumers depend on the headers target
`:<dir>`; only the linking target (`//brave/browser:core`, through
`brave_chrome_browser_deps`) depends on `:impl`. Use a single target with
`public = [...]` and `sources = [...]` only for a small module with few
consumers.

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
    "//chrome/test/base:in_process_browser_test",
  ]
}
```

- Every target depends directly on each target whose headers its own files
  include. Do not rely on a header arriving through another target's
  `public_deps`: upstream is enabling `check_includes_strict` directory by
  directory, and it rejects that.
- Use `public_deps` only for targets whose headers this target's public headers
  include. Everything else goes in `deps`.
- In test targets, depend on the specific `//chrome/test/base:*` targets
  (`in_process_browser_test`, `ui_test_utils`, `testing_profile`, ...), not on
  the `//chrome/test:test_support` or `:test_support_ui` aggregate groups.
- Factory `.h` files go in the headers target; their `.cc` goes in `:impl`.
- Keep every `if (is_android)`, `if (toolkit_views)` and buildflag guard around
  the moved files identical to the original, and guard each dep like the file
  that uses it.
- Tests go in `:unit_tests` and `:browser_tests`; `:browser_tests` sets
  `defines = [ "HAS_OUT_OF_PROC_TEST_RUNNER" ]`.
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
  needs its upstream target to depend on the narrow `:<dir>` headers target,
  never on `//brave/browser`, through the matching `sources.gni` deps list or a
  `rewrite/` substitution. A dep on a broad target turns one edge into a cycle.

### 4. Fix `gn check` errors

- "Include not allowed" or "can't include this header from here": add the
  missing dep directly to the target whose file has the include. Move it to
  `public_deps` only when the include is in that target's public headers.
  `gn check --fix --error-limit=-1 out/<dir>` applies its suggestions in one
  pass; review the result, since it can add duplicate entries.
- An error that would need a new `allow_circular_includes_from` entry means the
  file is blocked (see Rules). Do not add the entry.
- Once the files are out, check the circular lists for entries that existed only
  for them and remove them with [B].

## [B] Remove an `allow_circular_includes_from` entry

1. Remove the entry from the list. If the declaring target has no entries left,
   remove the key and remove the target from `circular_includes_brave.gni`.
2. Run `gn gen out/<dir>` and `gn check out/<dir>` on every directory.
3. Apply the first case that matches:

   - **Dead entry**: `A` does not transitively depend on `B`, or `B` does not
     include `A`. Run `gn check --fix`.
   - **Header in a monolith**: `B` includes a header that is in `A` only because
     `A` is a monolith (`brave_chrome_browser_sources`, `//chrome/browser:core`,
     `//chrome/browser/ui:ui`). For a Brave header, run [A] on its directory and
     retry. For an upstream header, restore the entry, keep or add a comment
     naming the header, and report it to the user as blocked.
   - **Real cycle**: `A` includes `B` and `B` includes `A`. Fix the design first
     (see "Breaking a design cycle"); otherwise extract the shared declarations
     into a third headers target, or merge the two targets.
   - **`component`/`static_library` with sources**: `A` is a link target with
     `public`/`sources`. Copy `A` into `source_set("A_sources")` (or
     `static_library("A_sources")`), turn `A` into a `group` (a `static_library`
     without sources and configs if it must stay one), add `A_sources` to `A`'s
     `public_deps`, remove `deps` and `public_deps` from `A_sources`, then let
     `gn check --fix` add the deps `A_sources` needs and add `A_sources` to `B`.
   - **Backwards arrow**: `B` includes `A`, and `A` depends on `B` without
     including it. Run `gn check --fix` so `B` depends on `A`, run `gn gen`,
     then delete the edge in the reported loop that has no matching `#include`.

4. If `gn check --fix` reports a visibility error, fix the visibility or choose
   the correct target.

## Breaking a design cycle

Prefer fixing the cause over relocating files:

- **`Browser*` / `BrowserView*`**: pass what is needed (`PrefService*`,
  `TabInterface&`, `BrowserWindowInterface*`) in the constructor. Do not look up
  a window with global lookups such as
  `BrowserCollection::FindBrowserWithTab()`; use
  `tab->GetBrowserWindowInterface()`.
- **Feature state**: expose it through `UnownedUserData` (`X::From(tab)`) so
  consumers depend on the headers target of `X`, not on its owner
  (`BraveTabFeatures`, `BrowserWindowFeatures`).
- **Push vs pull**: a coordinator that both queries its children and is called
  back by them forms a cycle. Pick one direction.
- **Subclassing an upstream class from `//brave`**: make the Brave class a
  template over its base, so the Brave target does not depend on the upstream
  target. Only the upstream target depends on the Brave one, through the
  `chromium_src` override that swaps the class in:

  ```cpp
  // brave/browser/foo/brave_foo.h
  template <typename ChromeFoo>
  class BraveFoo : public ChromeFoo {
    ...
  };

  // chromium_src/chrome/browser/foo/foo_owner.cc
  #include "brave/browser/foo/brave_foo.h"
  // Included before the define so the declaration of ChromeFoo is not renamed.
  #include "chrome/browser/foo/chrome_foo.h"
  #define ChromeFoo BraveFoo<ChromeFoo>
  #include <chrome/browser/foo/foo_owner.cc>
  #undef ChromeFoo
  ```

## Verify

- `gn check` passes on every directory.
- [B] with metadata-only changes: `siso query graph` after the edit matches the
  baseline (only `A`→`A_sources` object renames are expected when splitting).
- Build with
  `pnpm run build --ignore_compile_failure --target=brave:all --gn=enable_updater:true --gn=use_prebuilt_omaha4:false`.
  Moving sources or turning `public_deps` into `deps` can pass `gn check` and
  still fail at link time.
- [A]: run the moved tests, e.g.
  `pnpm run test brave_unit_tests --filter="Foo*"` and
  `pnpm run test brave_browser_tests --filter="Foo*"`, and confirm the same
  number of tests runs as before the move.

## Commit message (when asked to commit)

Follow the `cl-description` conventions, with a `Bug:` footer linking the
brave-browser issue the change is for.

- [A] Subject: `Modularize //brave/browser/<dir>`. Body: which targets now exist
  and what left the monolith.
- [B] Subject: `Remove allow_circular_includes_from from //path:target`. Body:
  why the entry existed, then how it was removed.
