# `tools/cr`

Python tooling maintained by the Chromium Rebase team to support upgrading
`brave-core` to new Chromium versions and to manage the patches and shadow files
that live on top of upstream.

All scripts here are intended to run via `vpython3`; the `.vpython3` spec in
this directory pulls in `rich`, `requests`, and the other wheels they need. Most
scripts accept `--verbose` to switch logging to `DEBUG`.

> [!IMPORTANT]
>
> Before adding or editing Python in this directory, read
> [Python dos and donts](python_dos_and_donts.md).

## Entry points

These are the user-facing scripts in this directory:

- **🚀Brockit** — End-to-end rebase orchestration. Updates the Chromium version,
  regenerates patches and strings, and walks the rebase forward.
- **🩹Plaster** — Applies and regenerates "plaster" files: semantic patches
  against upstream Chromium sources.
- `post_presubmit.py` — Posts and refreshes GitHub PR comments from a
  `npm run presubmit` JSON dump.
- `find_rebase_commits.py` — Resolves a Chromium rebase tag (e.g. `cr148`) to an
  `oldest..newest` git commit range.
- `prune_test_filters.py` — Detects and (optionally) removes obsolete entries
  from the current platform's test-filter files.

## Library modules

Shared abstractions used by the entry points and tests:

- `repository.py` — `Repository` instances (`repository.brave`,
  `repository.chromium`) for paths and for running git commands at any repo.
- `terminal.py` — Rich console, baseline logging config, and subprocess
  utilities.
- `vpython_utils.py` — Provides `vpython3` path.
- `patchfile.py` — Patch file abstraction handling patches that failed to apply.
- `git_status.py` — Parser for `git status --porcelain` output.
- `versioning.py` — Chromium version number parsing.
- `vscode.py` — IPC for opening files in VS Code in the integrated terminal.

## gclient hooks

- `install_extra_deps.py` — a gclient hook (wired up from `DEPS`) that installs
  archives declared in the `EXTRA_DEPS` table. It downloads the archive,
  verifies its `sha256sum`, and extracts it into the destination path..

  Dependencies come into ownership flavours:

  - **Overlay** (with `overlayed_on`): the archive lays its members out relative
    to the destination root and is extracted straight on top of the existing
    upstream tree named by `overlayed_on` (validated against `DEPS`, so we never
    overlay onto a rolled base).
  - **Owned** (without `overlayed_on`): the object fully owns its destination
    directory, which is wiped and re-extracted on every install so nothing from
    a prior install lingers.

  Downloads are declared as follows:

  ```python
  EXTRA_DEPS = {
    'src/path/to/install/dir': {
      'bucket': 'https://<bucket>/<prefix>/',
      'condition': '<optional dep-level condition>',
      'objects': [
        {
          'object_name': '<archive-name>.tar.xz',
          'sha256sum': '<hex sha256>',
          # 'overlayed_on': '<upstream archive>',  # omit for an owned dep
          'condition': 'host_os == "linux"',  # one matching object per host
        },
      ],
    },
  }
  ```

  Invoke with the `sync` subcommand and the target path, e.g.
  `install_extra_deps.py sync src/path/to/install/dir`.

  Repin an entry in place with the `setdep` subcommand, which rewrites the
  archive's `object_name`, `sha256sum`, `size_bytes`, and (optionally)
  `overlayed_on`, while preserving every comment, blank line, and quote style in
  the `EXTRA_DEPS` file:

  ```sh
  install_extra_deps.py setdep \
    -r src/path/to/install/dir@<archive>.tar.xz,<hex sha256>,<size_bytes>
  ```

  For an overlay entry, append the new upstream archive it sits on as a fourth
  field:

  ```sh
  install_extra_deps.py setdep \
    -r src/path/to/install/dir@<archive>.tar.xz,<hex sha256>,<size_bytes>,<upstream archive>
  ```

  Join a multi-object (e.g. per-platform) entry's objects with `?`, in the
  entry's existing order. Repeat `-r` to repin several entries at once. The
  object count must match the entry's current count.

## Subdirectories

- `alias/` — `git cr` git-alias subcommands (`commit`, `mv`, `follow-renames`,
  ...). See [`alias/README.md`](alias/README.md).
- `bootstrap/` — An experiment to boostrap some of our python utils in the
  user's PATH.
- `test/` — Shared test fixtures:
  - `fake_chromium_repo.py` — `FakeChromiumRepo`, a real git checkout shaped
    like `src/` with brave inside it, plus an emulation of the `npm run` build
    commands (`init`, `apply_patches`, `update_patches`, `chromium_rebase_l10n`,
    `gnrt`) over it.
  - `fake_terminal.py` — `FakeTerminal`, which routes `terminal.run` so that git
    runs for real against the fake checkout while everything else is emulated,
    and `ConsoleCapture`, which captures what a command prints.
  - `fake_gh.py` — `FakeGh`, a stand-in for the `gh` CLI.
- `toolchain/` — Scripts to build platform-specific toolchains when rebasing
  Chromium.

## Tests

Every module of interest ships a `*_test.py` next to it. Tests should always
have a `vpython3` shebang line, and `+x` mode, so they can be run as their own
utilities from the repo root.

Tests should keep mock use to a minimum. Make sure to always use
`FakeChromiumRepo` to provide a test enviroment. Scripts tested with
`FakeChromiumRepo` should rely always on relative paths derived from
`Repository.brave.root` to permit tests to be run in integration with the
repository sandbox provided with `FakeChromiumRepo`.

A command can also be driven end to end, with `main()` called on an argument
list and only the things a fake checkout cannot serve emulated (see the `test/`
fixtures above). `brockit_lift_test.py` does this for `brockit.py lift`, and is
the place to look for the pattern.
