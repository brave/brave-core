# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""bots gen: write a builder's `args.gn` and run `gn gen`.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess

import gen_paths
import generated_output

_CHROMIUM_SRC_DIR = gen_paths.BOTS_DIR.parents[2]


class BuildDirGenerator(generated_output.OutputGenerator):
    """Writes a builder's `args.gn` (and a secrets stub) and runs `gn gen`.
    """

    def __init__(self, builder_name: str, out_dir: Path | None = None) -> None:
        super().__init__(builder_name)
        self.out_dir = out_dir if out_dir is not None else self.default_out_dir(
        )

    def default_out_dir(self) -> Path:
        """The output directory this builder gets when none is given.
        """
        return _CHROMIUM_SRC_DIR / 'out' / self.builder_name

    def secrets_import_path(self) -> str:
        """Overrides `OutputGenerator`'s default: the secrets stub this
        generator writes (`write_build_dir()`) lands in `self.out_dir`,
        which is not necessarily the default `out/<builder>` once
        `--out-dir` overrides it, so the import must follow it there.
        """
        return '//%s/brave_secrets.gni' % self.out_dir.relative_to(
            _CHROMIUM_SRC_DIR).as_posix()

    @staticmethod
    def render_secrets_gni_stub(secrets: dict[str, str]) -> str:
        """Renders a placeholder `brave_secrets.gni` for a `secrets` map.

        This stub exists so a builder that declares secrets still gets a
        `gn gen` that succeeds. Every declared GN arg gets a "dummy" value, the
        same placeholder `gn_args.py` already uses for `brave_google_api_key`.
        """
        return ''.join('%s = "dummy"\n' % name for name in sorted(secrets))

    def write_build_dir(self) -> str:
        """Writes `args.gn` (and a secrets stub, if the builder declares
        any) into `self.out_dir`, creating it if needed.

        Returns:
            The rendered `args.gn` text.
        """
        resolved = self.read_generated_gn_args()
        args_gn = self.render_args_gn(resolved)

        self.out_dir.mkdir(parents=True, exist_ok=True)
        (self.out_dir / 'args.gn').write_text(args_gn,
                                              encoding='utf-8',
                                              newline='\n')

        secrets = resolved.get('secrets')
        if secrets:
            (self.out_dir / 'brave_secrets.gni').write_text(
                self.render_secrets_gni_stub(secrets),
                encoding='utf-8',
                newline='\n')

        return args_gn

    def run_gn_gen(self) -> int:
        """Runs `gn gen` against `self.out_dir`.

        `args.gn` is already on disk (`write_build_dir`), so, as `mb.py gen`
        does, this only ever needs to name the directory; `--check` runs the
        header checker over the whole graph, matching `mb.py`'s own default.
        GN resolves the build directory against the source root it finds
        from the working directory, so this always runs from
        `_CHROMIUM_SRC_DIR`.
        """
        return subprocess.call([
            'gn', 'gen',
            str(self.out_dir.relative_to(_CHROMIUM_SRC_DIR)), '--check'
        ],
                               cwd=_CHROMIUM_SRC_DIR)

    def gen(self) -> int:
        """Writes the build directory, then runs `gn gen` against it."""
        self.write_build_dir()
        return self.run_gn_gen()


def cmd_gen(args: argparse.Namespace) -> int:
    out_dir = Path(args.out_dir).resolve() if args.out_dir else None
    generator = BuildDirGenerator(args.builder, out_dir)
    if not generator.out_dir.is_relative_to(_CHROMIUM_SRC_DIR):
        raise generated_output.BotsError(
            '--out-dir %s is not under the chromium checkout root (%s); '
            'gn requires a build directory inside the source tree.' %
            (generator.out_dir, _CHROMIUM_SRC_DIR))
    return generator.gen()


def add_subparser(subparsers) -> argparse.ArgumentParser:
    """Registers the `gen` subcommand onto `bots.py`'s subparsers."""
    gen_parser = subparsers.add_parser(
        'gen', description="Write a builder's args.gn and run `gn gen`.")
    generated_output.add_builder_argument(gen_parser,
                                          help_text='A builder name, e.g. '
                                          '"linux-x64-asan-brave".')
    gen_parser.add_argument(
        '--out-dir',
        help='Where to write args.gn and run `gn gen` (default: '
        'out/<builder>, under the chromium checkout root).')
    gen_parser.set_defaults(func=cmd_gen)
    return gen_parser
