# `tools/cr/bootstrap`

Provides shims for `plaster` and `brockit`, selecting them relative to the Brave
repository in the current directory.

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

These shims are resolved relative to `brave-core`, so attempting to call these
from a directory that is not inside `brave-core` will result in a failure.

## How it works

- `brockit`, `plaster` (POSIX) and `brockit.bat`, `plaster.bat` (Windows) are
  thin wrappers that call `launcher.py <tool> <args…>`.
- `launcher.py` resolves the brave-core root via `git rev-parse --show-toplevel`
  (validating the root is a `brave` work tree, matching `repository.py`), then
  execs `vpython3 <root>/tools/cr/<tool>.py <args…>` **without changing the
  cwd**, so the tool runs relative to your current path.
- `bootstrap.py` installs/uninstalls this directory on `$PATH`.
