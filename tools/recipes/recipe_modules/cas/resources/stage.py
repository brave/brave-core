#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Stage files and directories as one tree, for `remotetool` to upload.

Each path is relative to `--cwd`, and keeps its place relative to `--root` in
the staged tree, so `out/Release/../../brave/test/data/` is staged as
`brave/test/data/`. Files are hard-linked where they can be, and copied where
they cannot, such as across devices.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
from collections.abc import Iterator
from pathlib import Path


def staged_paths(root: Path, cwd: Path, paths: list[str]) -> Iterator[str]:
    """Each file *paths* hold, relative to *root*.

    Raises:
        ValueError: A path is outside *root*, or does not exist.
    """
    for path in paths:
        source = Path(os.path.normpath(cwd / path))
        try:
            relative = source.relative_to(root)
        except ValueError:
            raise ValueError(f'{path} is outside {root}') from None
        if source.is_dir():
            for dirpath, _, filenames in os.walk(source):
                for filename in filenames:
                    yield (
                        (Path(dirpath) / filename).relative_to(root).as_posix()
                    )
        elif source.is_file():
            yield relative.as_posix()
        else:
            raise ValueError(f'{path} does not exist')


def stage(root: Path, cwd: Path, paths: list[str], destination: Path) -> int:
    """Stages *paths* under *destination*.

    Returns:
        How many files were staged.
    """
    files = sorted(set(staged_paths(root, cwd, paths)))
    for file in files:
        target = destination / file
        target.parent.mkdir(parents=True, exist_ok=True)
        try:
            os.link(root / file, target)
        except OSError:
            shutil.copy2(root / file, target)
    return len(files)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument(
        '--cwd',
        type=Path,
        help='What the paths are relative to; --root if unset.',
    )
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument(
        '--paths-json',
        type=Path,
        required=True,
        help='A JSON list of the paths to stage.',
    )
    args = parser.parse_args(argv)

    root = args.root.resolve()
    cwd = (args.cwd or args.root).resolve()
    paths = json.loads(args.paths_json.read_text(encoding='utf-8'))
    try:
        count = stage(root, cwd, paths, args.stage)
    except ValueError as e:
        print(e, file=sys.stderr)
        return 1
    print(f'Staged {count} files')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
