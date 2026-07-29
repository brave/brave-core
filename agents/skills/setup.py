#!/usr/bin/env python3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Link agent skills into the Claude Code discovery dir.

The source of truth for Brave's skills is ``src/brave/agents/skills/<name>/``
(versioned and reviewed, mirroring chromium's ``//agents/skills/``). But Claude
Code only discovers skills under a ``skills/`` directory inside a ``.claude``
config dir. This script bridges the two by creating one symlink per skill:

    .claude/skills/<name>  ->  agents/skills/<name>

Claude Code follows the symlink, reads ``SKILL.md`` from the target, and
de-dupes if the same skill is reachable twice.

Upstream Chromium ships its own skills under ``src/agents/skills/`` (the parent
checkout, one level up from ``src/brave``). Those are linked too by default, so
Chromium skills such as ``browser-window-feature-refactor`` are usable on the
Brave codebase without a separate step; pass ``--no-upstream`` to skip them.
Brave's own skills win a name collision, so a vendored copy of an upstream skill
(e.g. one not yet in the pinned Chromium version) overrides the upstream one
until the vendored copy is dropped.

It is intended to be run automatically on every ``npm run sync`` (see the sync
hook that invokes it), so no developer has to run it by hand and the generated
links are never committed. It is idempotent: re-running only creates links that
are missing or wrong, and never clobbers a real (non-link) directory a developer
may have placed there.

On Windows we COPY instead of symlink, because git's symlink support on Windows
is unreliable and creating symlinks there often needs elevated privileges.

Usage:
    python3 agents/skills/setup.py link  # project .claude/skills (default)
    python3 agents/skills/setup.py link --user  # ~/.claude/skills
    python3 agents/skills/setup.py link --no-upstream  # Brave skills only
    python3 agents/skills/setup.py list  # show source vs. linked state
    python3 agents/skills/setup.py unlink  # remove only the links we own
"""

import argparse
import logging
import os
import shutil
import sys
from pathlib import Path

# .../src/brave/agents/skills/setup.py -> parents[2] == .../src/brave
_BRAVE_SRC = Path(__file__).resolve().parents[2]
_SKILLS_SRC = _BRAVE_SRC / 'agents' / 'skills'
# Upstream Chromium skills live in the parent checkout at src/agents/skills.
_UPSTREAM_SKILLS_SRC = _BRAVE_SRC.parent / 'agents' / 'skills'
_IS_WINDOWS = os.name == 'nt'


def _log(msg, *args):
    logging.info(msg, *args)


def _discovery_dir(user: bool) -> Path:
    """Where Claude Code looks for skills."""
    if user:
        return Path.home() / '.claude' / 'skills'
    return _BRAVE_SRC / '.claude' / 'skills'


def _is_skill_dir(path: Path) -> bool:
    """A dir is a skill if it holds SKILL.md (accept lowercase skill.md too)."""
    return (path / 'SKILL.md').exists() or (path / 'skill.md').exists()


def _prune_caches(root: Path) -> bool:
    """Delete __pycache__ dirs under `root`, then any dirs left empty by that.

    Touches only generated caches — never a source file. Returns True when
    `root` ends up empty (i.e. it held nothing but caches), meaning it is safe
    to reclaim. Processes deepest paths first so a parent like scripts/ is
    re-checked and removed after its own __pycache__ child is gone.
    """
    for path in sorted(root.rglob('*'),
                       key=lambda p: len(p.parts),
                       reverse=True):
        if not path.is_dir():
            continue
        if path.name == '__pycache__':
            shutil.rmtree(path, ignore_errors=True)
        elif not any(path.iterdir()):
            path.rmdir()
    return not any(root.iterdir())


def _scan_skills(root: Path) -> dict[str, Path]:
    """Map skill name -> source dir for every skill directly under ``root``.

    A directory is a skill if it contains SKILL.md (accept lowercase skill.md
    too — Claude Code discovers both). Returns {} if ``root`` does not exist.
    """
    skills: dict[str, Path] = {}
    if not root.is_dir():
        return skills
    for entry in sorted(root.iterdir()):
        if not entry.is_dir():
            continue
        if _is_skill_dir(entry):
            skills[entry.name] = entry
    return skills


def available_skills(upstream: bool = True) -> dict[str, Path]:
    """Map skill name -> source dir for every discoverable skill.

    Scans upstream Chromium's skills first (when ``upstream`` is set and the
    parent checkout has them), then Brave's own. Brave's ``dict.update`` runs
    last so a Brave skill wins a name collision with an upstream one.
    """
    skills: dict[str, Path] = {}
    if upstream:
        skills.update(_scan_skills(_UPSTREAM_SKILLS_SRC))
    skills.update(_scan_skills(_SKILLS_SRC))
    return skills


def _link_one(name: str, src: Path, dest: Path) -> tuple[bool, bool]:
    """Create/refresh one link (or copy on Windows).

    Returns (ok, changed). `changed` is False when the link was already correct
    or a real dir was left alone, so a routine sync — where every skill is
    already linked — has nothing to report and stays silent.
    """
    # Already correctly linked to our source? Nothing to do.
    if dest.is_symlink():
        try:
            if dest.resolve() == src.resolve():
                return True, False
        except OSError:
            pass  # broken symlink — replace it below
        _log('  refreshing stale link: %s', name)
        dest.unlink()
    elif dest.exists():
        # A real directory/file is sitting where we'd link. On Windows this is
        # our own previous copy; refresh it. Elsewhere it is either a migration
        # leftover git could not prune (a dir holding only __pycache__ from a
        # prior run) or a developer's real skill/WIP. Only reclaim the former;
        # _prune_caches removes caches and reports whether the dir is now empty,
        # so any real file makes us skip and preserve it.
        if _IS_WINDOWS:
            shutil.rmtree(dest, ignore_errors=True)
        elif dest.is_dir() and _prune_caches(dest):
            # The dir held nothing but generated __pycache__ (a migration
            # leftover git could not prune). Caches are gone and it is now
            # empty, so remove the empty dir and link.
            _log('  reclaiming cache-only dir: %s', name)
            dest.rmdir()
        else:
            # Warning-level so it survives -q in the sync hook.
            logging.warning(
                '  SKIP %s — a real dir with content at %s '
                '(left as-is)', name, dest)
            return True, False

    dest.parent.mkdir(parents=True, exist_ok=True)
    if _IS_WINDOWS:
        shutil.copytree(src, dest)
        logging.debug('  copied  %s', name)
    else:
        # Relative target keeps the link valid if the checkout moves.
        os.symlink(os.path.relpath(src, dest.parent),
                   dest,
                   target_is_directory=True)
        logging.debug('  linked  %s', name)
    return True, True


def link_skills(user: bool, upstream: bool = True) -> bool:
    skills = available_skills(upstream)
    if not skills:
        _log('No skills found under %s — nothing to link.', _SKILLS_SRC)
        return True
    dest_root = _discovery_dir(user)
    logging.debug('Linking %d skill(s) into %s', len(skills), dest_root)
    ok = True
    changed = 0
    for name, src in skills.items():
        try:
            one_ok, one_changed = _link_one(name, src, dest_root / name)
            ok = one_ok and ok
            changed += one_changed
        except OSError as e:
            # Error-level so it survives -q in the sync hook.
            logging.error('  ERROR linking %s: %s', name, e)
            ok = False
    # Only speak up when we actually did something. A steady-state sync (every
    # skill already linked) prints nothing.
    if changed:
        _log('Linked %d skill(s) into %s', changed, dest_root)
    return ok


def unlink_skills(user: bool, upstream: bool = True) -> bool:
    """Remove only links pointing into a known skills dir (never real dirs)."""
    dest_root = _discovery_dir(user)
    if not dest_root.is_dir():
        return True
    for name, src in available_skills(upstream).items():
        dest = dest_root / name
        if dest.is_symlink():
            try:
                if dest.resolve() == src.resolve():
                    dest.unlink()
                    _log('  unlinked %s', name)
            except OSError:
                dest.unlink()  # broken link into our tree — safe to drop
        elif dest.exists() and _IS_WINDOWS:
            shutil.rmtree(dest, ignore_errors=True)
            _log('  removed copy %s', name)
    return True


def list_skills(user: bool, upstream: bool = True) -> bool:
    dest_root = _discovery_dir(user)
    skills = available_skills(upstream)
    width = max((len(n) for n in skills), default=5)
    print(f'brave source:    {_SKILLS_SRC}')
    print(f'upstream source: {_UPSTREAM_SKILLS_SRC}'
          f'{"" if _UPSTREAM_SKILLS_SRC.is_dir() else " (absent)"}')
    print(f'discovery:       {dest_root}\n')
    print(f'{"SKILL".ljust(width)}  ORIGIN    LINKED')
    print(f'{"-" * width}  --------  ------')
    for name, src in skills.items():
        origin = 'brave' if src.parent == _SKILLS_SRC else 'upstream'
        dest = dest_root / name
        state = 'no'
        if dest.is_symlink():
            state = 'link' if dest.exists() else 'BROKEN link'
        elif dest.exists():
            state = 'dir (copy/manual)'
        print(f'{name.ljust(width)}  {origin.ljust(8)}  {state}')
    return True


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command',
                        choices=['link', 'unlink', 'list'],
                        nargs='?',
                        default='link')
    parser.add_argument('--user',
                        action='store_true',
                        help='Target ~/.claude/skills instead of the project '
                        'src/brave/.claude/skills.')
    parser.add_argument('--no-upstream',
                        dest='upstream',
                        action='store_false',
                        help="Only Brave's own skills; skip upstream Chromium "
                        'skills under src/agents/skills.')
    parser.add_argument('-q',
                        '--quiet',
                        action='store_true',
                        help='Only log warnings/errors (for sync hooks).')
    args = parser.parse_args()

    logging.basicConfig(level=logging.WARNING if args.quiet else logging.INFO,
                        format='%(message)s')

    handler = {
        'link': link_skills,
        'unlink': unlink_skills,
        'list': list_skills
    }[args.command]
    return 0 if handler(args.user, args.upstream) else 1


if __name__ == '__main__':
    sys.exit(main())
