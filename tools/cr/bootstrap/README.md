# `tools/cr/bootstrap`

Provides shims for `plaster`, `brockit`, `node`, `npm` and `fetch_brave`,
selecting them relative to the Brave repository in the current directory.

`plaster` / `brockit` dispatch to the matching `tools/cr` script. `node` / `npm`
are a prototype front-end for the node delivered into `third_party/node` by
`gclient sync` (see [Node delivery](#node-delivery) below): inside a checkout
that has a downloaded node they run _that_ node, and everywhere else they fall
back transparently to the system `node` / `npm`. `fetch_brave` runs
`tools/cr/fetch_brave.py` from _this_ bootstrap's own checkout to set up a
brand-new checkout under the current directory — so once the bootstrap is
installed you can `mkdir new-browser && cd new-browser && fetch_brave` without
re-fetching the script.

## Install

```sh
# From any brave-core checkout:
vpython3 tools/cr/bootstrap/bootstrap.py install
```

On macOS / Linux this configures your current shell (override with
`--shell bash|zsh|fish|all`):

- **bash** → a marked block in `~/.bashrc`
- **zsh** → a marked block in `~/.zshrc`
- **fish** → a drop-in at `~/.config/fish/conf.d/brave-bootstrap.fish` that
  prepends to `$PATH` directly (it avoids `fish_add_path`, which would persist
  the entry in the universal `fish_user_paths` variable and outlive the file)

On Windows it adds this directory to the user `Path` under `HKCU\Environment`.

In every case the directory is placed **first** on `$PATH`, so these shims take
precedence over any other `brockit` / `plaster` on the system.

**Open a new shell for the change to take effect.**

## Uninstall

```sh
vpython3 tools/cr/bootstrap/bootstrap.py uninstall
```

Removes the managed block / drop-in from every known shell location (POSIX) or
the `Path` entry (Windows). For fish it also scrubs any legacy entry left in the
universal `fish_user_paths` variable by older versions of this script.

The directory removed from `$PATH` is the one the live shim actually resolves to
(via `shutil.which`), so `uninstall` works even when run from a different
checkout than the one that was installed.

## Usage

```sh
cd ~/browser/src/brave/components # anywhere inside a checkout
plaster check                     # runs that checkout's plaster.py
brockit lift --to=1.2.3.4         # runs that checkout's brockit.py
```

`plaster` / `brockit` are resolved relative to `brave-core`, so calling them
from a directory that is not inside `brave-core` will fail. `node` / `npm`
instead fall back to the system tool in that case (see below).

```sh
cd ~/browser/src/brave
node --version                    # runs third_party/node's node
cd /tmp
node --version                    # falls back to the system node
```

## Node delivery

The `download_node` hook in `brave/DEPS` runs
`third_party/node/download_node.py` during `gclient sync`. As a prototype, it
downloads the official Node.js distribution (which **includes** npm — unlike
Chromium's `third_party/node`, which strips it) straight from `nodejs.org/dist`,
verifies it against the published `SHASUMS256.txt`, and extracts it into
`third_party/node/<platform>/node-<suffix>/` with the version stripped from the
directory name. The download is idempotent via a `.brave_node_version` stamp.

## How it works

- `brockit`, `plaster`, `node`, `npm`, `fetch_brave` (POSIX) and their `.bat`
  variants (Windows) are thin wrappers that call `launcher.py <tool> <args…>`.
- `launcher.SHIM_TARGETS` is the single source of truth for what each shim runs:
  a flat map of tool name → path relative to the **workspace root** (the parent
  of `src`). Each path is explicit about its origin — brave-owned targets go
  through `src/brave/…`, so chromium-owned scripts under `src/…` can be shimmed
  in future on the same base. node/npm are keyed per platform as `node-<plat>` /
  `npm-<plat>` (e.g. `node-win`).
- Every cwd-relative tool finds the checkout the same way: by the
  `<workspace>/src/brave` path layout (`find_brave_checkout`), not `git`. A
  single rule covers being inside the checkout, inside a nested DEPS repo such
  as `vendor/web-discovery-project` (where `git rev-parse` would report the
  nested repo, not brave), and above `src/brave`. Targets then resolve from the
  workspace root (the checkout's grandparent). The cwd is never changed, so
  tools run relative to your current path.
- For `brockit` / `plaster`, `launcher.py` execs
  `vpython3 <workspace>/<SHIM_TARGETS[tool]>`, erroring when the cwd is not
  inside a checkout.
- For `node` / `npm`, the shim's name carries the platform: the `.bat` shims
  pass the qualified `node-win` / `npm-win`, while the POSIX shims pass the bare
  `node` / `npm` and the launcher appends the host key (`linux` / `mac` /
  `mac_arm64`). `launcher.py` then runs the checkout's downloaded node at that
  `SHIM_TARGETS` entry when present, otherwise falls back to the system tool —
  searching `$PATH` with this directory removed so the shim never recurses into
  itself. npm is launched as `<repo node> npm-cli.js` to avoid the
  `#!/usr/bin/env node` lookup re-entering the shim.
- For `fetch_brave`, `launcher.py` runs `SHIM_TARGETS['fetch_brave']` resolved
  relative to **this launcher's own location** (not the cwd or git) under the
  same interpreter, since it bootstraps a new checkout into an empty directory
  with no checkout to detect and no `vpython3` yet.
- `bootstrap.py` installs/uninstalls this directory on `$PATH`. The `node` /
  `npm` shims are deliberately **not** treated as install markers (a system
  `node` must not look like a prior install), but their files ship here and go
  live the moment this directory is on `$PATH`.
