# `git cr` alias for Chromium rebasing tooling

This alias is maintained by the Chromium Rebase team to simplify many of the
rebase tasks. This directory provides the `git cr` git alias and the supporting
commit-msg hook used when committing to `brave-core`.

## Quick start

From the `brave-core` repository root, install this alias with:

```sh
python3 tools/cr/alias/cmd.py setup-alias
git cr install-hook
```

* `setup-alias` writes a `cr` entry to `.git/config`. After this, `git cr`
  works in this repository.
* `install-hook` installs `commit-msg.py` as the repository's `commit-msg`
  hook. This is necessary to use `git cr commit`

Once installed, run `git cr` to see the available commands.

## Subcommands

### `git cr commit`

A wrapper around `git commit` that adds three convenience flags. All other
arguments are forwarded verbatim to `git commit`.

```sh
git cr commit -m "Fix login button alignment"
git cr commit --tagged=WIP,android -m "Work in progress"
git cr commit --issue 12345 -m "Resolve crash on launch"
git cr commit --culprit abc123,def456 -m "Adjust to upstream API change"
```

[commit-msg hook behaviour](#commit-msg-hook-behaviour) section below for
more details.

### `git cr mv`

Move a file or directory inside `brave-core` and repair every artefact that
references the old path.

```sh
git cr mv components/api_request_helper/ components/api_foo
npm run build -- --target=brave:all
```

What it repairs after the rename:

* C++ include guards in moved `.h` files (regenerated to match the new
  path).
* `#include <…>` shadow-include lines in moved `chromium_src/` files.
* `#include` / `#import` directives across `brave-core` (both quoted and
  angle-bracket forms, including `.mojom` files and their derived
  `*.mojom.h`, `*.mojom-blink.h`, etc.).
* `// path/to/file` style references inside C++ comments and `.gn`/`.gni`
  files.
* `BUILD.gn` / `.gni` source-list entries in the ancestor chain of the
  moved file.
* Quoted GN root references (`"//path/to/file"`) and relative GN
  references in `.gn` / `.gni` files.
* Plaster files (`rewrite/…/foo.h.toml`) and their associated patch files
  in `patches/`. The new plaster file is re-applied so `patches/` is
  refreshed.

### `git cr follow-renames`

This command has some similarities to `git-cr-mv`, however its main role is
to correct move operations that occurred in upstream Chromium.

```sh
# All renames between two Chromium version tags (typical version bump):
git cr follow-renames 149.0.7827.1..149.0.7827.4

# Renames introduced by a single upstream commit:
git cr follow-renames 4f093f4239eb2814f57bc97ee593d7acd717ac42
```

This command does all the rewrites done by `git-cr-mv`, but it also does a few
extra things:

1. Move `chromium_src/<old>` to `chromium_src/<new>` and refresh its
   include guard and shadow `#include` line.
2. Move `rewrite/<old>.toml` to `rewrite/<new>.toml`, delete the stale
   `patches/…` patch file (and `.patchinfo`), then re-run plaster on the
   new TOML so a fresh patch is produced.
3. Update every `#include`, `#import`, `// comment`, `BUILD.gn`, and `.gni`
   reference across `brave-core` (same logic as `git cr mv`).
4. For any patch that is **not** managed by plaster, rewrite the patch's
   `--- a/old`, `+++ b/new`, and `diff --git` lines, rename the file, and
   try `git apply --3way` against the Chromium tree so conflicts surface
   immediately.

## Why the use of a git commit message hook

Chromium rebases usually involve hundreds of fixes to Brave. These fixes are
the result of the way Chromium evolves. Therefore, `cr` have to provide a
significant amount of information regading the project history when introducing
these fixes. This is vital when trying to understand when and why certain
changes were introduced in the project.

The hook provides several helpers for commit messages that are relevant to
practices we have for `brave-core`. Check `commit-msg.py` for the full
documentation. A few examples that we use it for:

```sh
(branch: cr149) $ git commit -m "Disables kVerticalTabsLaunch feature."
# Resulting commit message:
# [cr149] Disables kVerticalTabsLaunch feature.
```

```sh
(branch: canary+fix-toolbar-crash) $ git commit -m "Guard against null TabStripModel"
# Resulting commit message:
# [canary] Guard against null TabStripModel
```

```sh
(branch: cr149+fix-issue-55193) $ git commit -m "Migrate ECDSA_SHA384 patches to plaster"
# Resulting commit message:
# [cr149] Migrate ECDSA_SHA384 patches to plaster
#
# Resolves https://github.com/brave/brave-browser/issues/55193
```

### Every commit on a `cr` is expected to have a `[crXXX]` tag

The main covenience the hook provides, is the enforcement of `[cr149]` prefix
when working on `cr` branches, which easily catches one's attention when
looking on a blame. This branch tag is important for any commit that is not
too obvious to be part of a `cr` branch without it.

### Using commit tags to provide context

We should always try to provide tags, as a way to group changes together.
For examnple, when a change only matters on a single platform, an OS tag could
(e.g. `[android]`, `[ios]`, etc) can provide important context when reviewing
the change, and also make it easier similar changes.

This is strongly encouraged. A single grep then yields every fix that
has touched the affected surface, and reviewers triaging a regression may be
able to take advantage of such extra context.

Adding additional tags is very simple:

```sh
(branch: cr149) $ git cr commit --tagged WIP -m "Guard against null TabStripModel"
# Resulting commit message:
# [cr149][WIP] Guard against null TabStripModel
```


### Each change should reference at least one upstream culprit

Rebase branch can grow to hundreds of changes. For reviewers, and future code
archeologists, it is important that we provide the exact cause for introducing
a change into the branch. The upstream changes driving our fixes are referred
to as **culprits**, and the tooling provides ways to keep them in our history.

`git cr commit --culprit <hash>` expands each hash into the commit message
of the fix being done. This allows us to have a record, but also to use
`git log --grep` to find changes based on CL numbers or `crbug` issue numbers.


Auto-generated upgrade messages (`Update from Chromium …`,
`Update patches from Chromium …`, `Updated strings for Chromium …`) are
left verbatim so they remain easy to grep.

We add culprits to commits using `git cr commit --culprit=[HASH,]`:

```sh
(branch: cr149) $ git cr commit \
    --culprit 70cb8d8433679502ede773d828bf31cb0a1bce16 \
    -m "Adapt to upstream proto changes"
# Resulting commit message:
# [cr149] Adapt to upstream proto changes
#
# Chromium changes:
# https://chromium.googlesource.com/chromium/src/+/70cb8d8433679502ede773d828bf31cb0a1bce16
#
# commit 70cb8d8433679502ede773d828bf31cb0a1bce16
# Author: Hamda Mare <hmare@google.com>
# Date:   Tue Apr 28 12:53:24 2026 -0700
#
#     Add proto and type definitions for certificate collection
#
#     This CL adds the necessary proto messages and C++ types to support
#     certificate collection in signal reports.
#
#     Bug: 502634772
#     Change-Id: I2136d39ac83c293856690a8acef12d6d2babdeec
#     Reviewed-on: https://chromium-review.googlesource.com/c/chromium/src/+/7795711
#     …
```
