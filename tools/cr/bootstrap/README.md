# `tools/cr/bootstrap`

Provides shims for `plaster`, `brockit`, `node` and `npm`, selecting them
relative to the Brave repository in the current directory.

`plaster` / `brockit` dispatch to the matching `tools/cr` script. `node` / `npm`
are a prototype front-end for the node delivered into `third_party/node` by
`gclient sync` (see [Node delivery](#node-delivery) below): inside a checkout
that has a downloaded node they run _that_ node, and everywhere else they fall
back transparently to the system `node` / `npm`.

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
