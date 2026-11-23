# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""bots snapshot: write `infra/config` builders matrix."""

from __future__ import annotations

import argparse
import dataclasses
import importlib
from pathlib import Path
import pkgutil
import sys

import gen_paths
import generated_output


def _load_config() -> None:
    """Imports gn_args.py, all targets/*.py and all builders/*.py."""
    config_dir = str(gen_paths.CONFIG_DIR)
    if config_dir not in sys.path:
        sys.path.insert(0, config_dir)
    importlib.import_module('gn_args')
    import targets as targets_package

    # Before the builders, which name the bundles these declare.
    for module_info in pkgutil.iter_modules(targets_package.__path__):
        importlib.import_module('targets.' + module_info.name)
    import builders as builders_package

    for module_info in pkgutil.iter_modules(builders_package.__path__):
        importlib.import_module('builders.' + module_info.name)


def compute_fresh_output(builders_registry, gn_args_registry) -> dict[str, str]:
    """Returns `{relative_path: content}` for every registered builder.

    Raises:
        ConfigError: a builder cannot be rendered (see
            `generated_output.render()`).
    """
    return {
        f'{builder.name}/{filename}': content
        for builder in builders_registry.all()
        for filename, content in generated_output.render(
            builder, gn_args_registry
        ).items()
    }


@dataclasses.dataclass(frozen=True)
class SnapshotResult:
    """What a `write_output()` call did, or would do under `dry_run`."""

    # Paths written because they were missing or didn't byte-match the fresh
    # content.
    changed: list[str]

    # Paths already byte-identical to the fresh content; left untouched.
    unchanged: list[str]

    # Paths that existed under the output dir but aren't in the fresh output
    # anymore; removed as stale.
    deleted: list[str]


def _scan_existing(output_dir: Path) -> set[str]:
    if not output_dir.is_dir():
        return set()
    return {
        p.relative_to(output_dir).as_posix()
        for p in output_dir.rglob('*')
        if p.is_file()
    }


def write_output(
    output_dir: Path, fresh: dict[str, str], *, dry_run: bool = False
) -> SnapshotResult:
    """Reconciles `output_dir` with `fresh` (`{relative_path: content}`).

    Writes the output dirs, with the builders, and makes sure that freshness/
    staleness is managed too. With `dry_run=True` no disk writes are made.
    """
    existing = _scan_existing(output_dir)
    deleted = sorted(existing - fresh.keys())

    changed = []
    unchanged = []
    for rel_path, content in fresh.items():
        path = output_dir / rel_path
        current = path.read_text(encoding='utf-8') if path.is_file() else None
        (unchanged if current == content else changed).append(rel_path)
    changed.sort()
    unchanged.sort()

    if not dry_run:
        for rel_path in deleted:
            (output_dir / rel_path).unlink()
        for rel_path in changed:
            path = output_dir / rel_path
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(fresh[rel_path], encoding='utf-8')

    return SnapshotResult(changed=changed, unchanged=unchanged, deleted=deleted)


def write_snapshot(
    builders_registry,
    gn_args_registry,
    output_dir: Path,
    *,
    dry_run: bool = False,
) -> SnapshotResult:
    """Resolves every registered builder and reconciles `output_dir` with it.

    Combines `compute_fresh_output()` and `write_output()`, so a caller never
    has to thread the intermediate `fresh` dict through itself.
    """
    fresh = compute_fresh_output(builders_registry, gn_args_registry)
    return write_output(output_dir, fresh, dry_run=dry_run)


def cmd_snapshot(args: argparse.Namespace) -> int:
    _load_config()
    from lib.config import builders, gn_args, targets

    stale = False
    for name, result in (
        (
            'builders',
            write_snapshot(
                builders,
                gn_args,
                gen_paths.BUILDERS_OUTPUT_DIR,
                dry_run=args.check,
            ),
        ),
        (
            'testing',
            write_output(
                gen_paths.TESTING_OUTPUT_DIR,
                {
                    'gn_isolate_map.pyl': generated_output.render_isolate_map(
                        targets
                    )
                },
                dry_run=args.check,
            ),
        ),
    ):
        prefix = '' if name == 'builders' else name + '/'
        for path in result.deleted:
            print(f'deleted:   {prefix}{path}')
        for path in result.changed:
            print(f'changed:   {prefix}{path}')
        if args.verbose:
            for path in result.unchanged:
                print(f'unchanged: {prefix}{path}')
        if args.check and (result.changed or result.deleted):
            print(
                f'infra/config/generated/{name}/ is stale; run '
                '`bots.py snapshot` to update it.',
                file=sys.stderr,
            )
            stale = True
    return 1 if stale else 0


def add_subparser(subparsers) -> argparse.ArgumentParser:
    """Registers the `snapshot` subcommand onto `bots.py`'s subparsers."""
    snapshot_parser = subparsers.add_parser(
        'snapshot',
        help='Write infra/config/generated/builders/ from the current spec.',
    )
    snapshot_parser.add_argument(
        '--check',
        action='store_true',
        help="Don't write anything; exit non-zero if snapshotting would "
        'change anything (for presubmit).',
    )
    snapshot_parser.add_argument('-v', '--verbose', action='store_true')
    snapshot_parser.set_defaults(func=cmd_snapshot)
    return snapshot_parser
