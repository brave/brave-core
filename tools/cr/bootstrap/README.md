# `tools/cr/bootstrap`

This path incubates a boostrap module, to provide shims as ergonomics for tools
we use to build Brave.

## Install

```sh
# From any brave-core checkout:
vpython3 tools/cr/bootstrap/bootstrap.py install
```

On macOS / Linux this configures your current shell (override with
`--shell bash|zsh|fish|all`):

- **bash** → a marked block in `~/.bashrc`
- **zsh** → a marked block in `~/.zshrc`
- **fish** → a drop-in at `~/.config/fish/conf.d/zzz-brave-bootstrap.fish` that
  prepends to `$PATH` directly.

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
git cr commit -m "…"              # runs that checkout's alias/cmd.py
```

`plaster` / `brockit` / `git cr` are resolved relative to `brave-core`, so
calling them from a directory that is not inside `brave-core` will fail. `node`
/ `npm` instead fall back to the system tool in that case (see below).

```sh
cd ~/browser/src/brave
node --version                    # runs third_party/node's node
cd /tmp
node --version                    # falls back to the system node
```

## The chicken-and-egg `node` problem

The shims for `node`/`npm` run cheap checks for the tarball installations, to
determine if a new file needs to be downloaded. This is done through the
`EXTRA_DEPS` mechanism itself, which makes this a lightweight sync check. For
repos with no mechanism for self-updating, the path resolution degrades back to
whatever is the system underlying binaries.
