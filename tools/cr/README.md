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

## Subdirectories

- `alias/` — `git cr` git-alias subcommands (`commit`, `mv`, `follow-renames`,
  ...). See [`alias/README.md`](alias/README.md).
- `bootstrap/` — An experiment to boostrap some of our python utils in the
  user's PATH.
- `test/` — Shared test fixtures, including `FakeChromiumRepo`.
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
