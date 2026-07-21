#!/usr/bin/env python3

# Copyright 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Link agent skills into the Claude Code discovery dir.

The source of truth for Brave's skills is ``src/brave/agents/skills/<name>/``
(versioned and reviewed, mirroring chromium's ``//agents/skills/``). Claude Code,
however, only discovers skills under a ``skills/`` directory inside a ``.claude``
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
    python3 agents/skills/setup.py link            # project .claude/skills (default)
    python3 agents/skills/setup.py link --user     # ~/.claude/skills
    python3 agents/skills/setup.py link --no-upstream  # Brave skills only
    python3 agents/skills/setup.py list            # show source vs. linked state
    python3 agents/skills/setup.py unlink          # remove only the links we own
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
        if (entry / 'SKILL.md').exists() or (entry / 'skill.md').exists():
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


def _link_one(name: str, src: Path, dest: Path) -> bool:
    """Create/refresh a single link (or copy on Windows). Returns True on success."""
    # Already correctly linked to our source? Nothing to do.
    if dest.is_symlink():
        try:
            if dest.resolve() == src.resolve():
                return True
        except OSError:
            pass  # broken symlink — replace it below
        _log('  refreshing stale link: %s', name)
        dest.unlink()
    elif dest.exists():
        # A real directory/file is sitting where we'd link. On Windows this is
        # our own previous copy; refresh it. Elsewhere it's unexpected (a hand
        # placed skill) — leave it and warn rather than destroy work.
        if _IS_WINDOWS:
            shutil.rmtree(dest, ignore_errors=True)
        else:
            _log('  SKIP %s — a non-symlink already exists at %s (left as-is)',
                 name, dest)
            return True

    dest.parent.mkdir(parents=True, exist_ok=True)
    if _IS_WINDOWS:
        shutil.copytree(src, dest)
        _log('  copied  %s', name)
    else:
        # Relative target keeps the link valid if the checkout moves.
        os.symlink(os.path.relpath(src, dest.parent), dest,
                   target_is_directory=True)
        _log('  linked  %s', name)
    return True


def link_skills(user: bool, upstream: bool = True) -> bool:
    skills = available_skills(upstream)
    if not skills:
        _log('No skills found under %s — nothing to link.', _SKILLS_SRC)
        return True
    dest_root = _discovery_dir(user)
    _log('Linking %d skill(s) into %s', len(skills), dest_root)
    ok = True
    for name, src in skills.items():
        try:
            ok = _link_one(name, src, dest_root / name) and ok
        except OSError as e:
            _log('  ERROR linking %s: %s', name, e)
            ok = False
    return ok


def unlink_skills(user: bool, upstream: bool = True) -> bool:
    """Remove only links that point back into a known skills dir (never real dirs)."""
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
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['link', 'unlink', 'list'],
                        nargs='?', default='link')
    parser.add_argument('--user', action='store_true',
                        help='Target ~/.claude/skills instead of the project '
                             'src/brave/.claude/skills.')
    parser.add_argument('--no-upstream', dest='upstream', action='store_false',
                        help="Only Brave's own skills; skip upstream Chromium "
                             'skills under src/agents/skills.')
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='Only log warnings/errors (for sync hooks).')
    args = parser.parse_args()

    logging.basicConfig(level=logging.WARNING if args.quiet else logging.INFO,
                        format='%(message)s')

    handler = {'link': link_skills, 'unlink': unlink_skills,
               'list': list_skills}[args.command]
    return 0 if handler(args.user, args.upstream) else 1


if __name__ == '__main__':
    sys.exit(main())
