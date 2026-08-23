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
import dotenv

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
        return '//%s/secrets.gni' % self.out_dir.relative_to(
            _CHROMIUM_SRC_DIR).as_posix()

    def resolve_secrets(self, secrets: dict[str, str]) -> dict[str, str]:
        """Resolves every declared secret's real value.

        This function reads the secrets from the `.env` file as usual and
        returns a dictionary for them.

        Raises:
            BotsError: the secrets `.env` file has no entry for one of the
                declared names.
        """
        env = dotenv.read()
        missing = sorted(name for name in secrets if name not in env)
        if missing:
            raise generated_output.BotsError(
                'builder %r declares secret(s) %s with no matching entry in '
                '%s.' %
                (self.builder_name, ', '.join(missing), dotenv.DEFAULT_PATH))
        return {name: env[name] for name in secrets}

    @staticmethod
    def render_secrets_gni(secrets: dict[str, str]) -> str:
        """Renders `secrets.gni` from resolved secret values."""
        return ''.join('%s = %s\n' %
                       (name, generated_output.gn_helpers.ToGNString(value))
                       for name, value in sorted(secrets.items()))

    def write_build_dir(self) -> str:
        """Writes `args.gn` and a resolved `secrets.gni` into `self.out_dir`.

        Returns:
            The rendered `args.gn` text.
        """
        resolved = self.read_generated_gn_args()
        declared_secrets = resolved.get('secrets')
        secrets = (self.resolve_secrets(declared_secrets)
                   if declared_secrets else None)

        args_gn = self.render_args_gn(resolved)

        self.out_dir.mkdir(parents=True, exist_ok=True)
        (self.out_dir / 'args.gn').write_text(args_gn,
                                              encoding='utf-8',
                                              newline='\n')

        if secrets:
            secrets_path = self.out_dir / 'secrets.gni'
            secrets_path.write_text(self.render_secrets_gni(secrets),
                                    encoding='utf-8',
                                    newline='\n')
            # Restrictive permissions: this file carries real secret values.
            secrets_path.chmod(0o600)

        return args_gn

    def run_gn_gen(self) -> int:
        """Runs `gn gen` against `self.out_dir`.
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
