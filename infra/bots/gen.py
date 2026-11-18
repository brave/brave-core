# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""bots gen: write a builder's build directory and run `gn gen`.

The build directory holds the builder's `args.gn` and, for the secrets it
declares, a `secrets.gni` the `args.gn` imports, with their real values.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys

import dotenv
import gen_paths
import generated_output

sys.path.insert(0, str(gen_paths.CHROMIUM_SRC_DIR / 'build'))
import gn_helpers  # pylint: disable=wrong-import-position


class BuildDir:
    """A builder's build directory, `out/<builder>` by default."""

    def __init__(self, builder_name: str, out_dir: Path | None = None) -> None:
        """
        Raises:
            BotsError: *out_dir* is not under the Chromium checkout, where gn
                needs a build directory to be.
        """
        self.builder_name = builder_name
        self.out_dir = (
            out_dir or gen_paths.CHROMIUM_SRC_DIR / 'out' / builder_name
        )
        if not self.out_dir.is_relative_to(gen_paths.CHROMIUM_SRC_DIR):
            raise generated_output.BotsError(
                '--out-dir %s is not under the chromium checkout root (%s); '
                'gn requires a build directory inside the source tree.'
                % (self.out_dir, gen_paths.CHROMIUM_SRC_DIR)
            )

    def render_args_gn(self) -> str:
        """The directory's `args.gn`. It imports the builder's secrets from
        the directory's `secrets.gni`.

        Raises:
            BotsError: the builder's generated output cannot be read.
        """
        return self._render_args_gn(self._read_gn_args())

    def write(self) -> str:
        """Writes the directory's `args.gn`, and its `secrets.gni` with the
        real values of the secrets the builder declares.

        The secrets are resolved before anything is written, so that a
        missing one leaves the directory as it was.

        Returns:
            The `args.gn` written.

        Raises:
            BotsError: the builder's generated output cannot be read, or a
                secret it declares has no value.
        """
        gn_args_json = self._read_gn_args()
        secrets = self._resolve_secrets(gn_args_json.get('secrets') or {})
        args_gn = self._render_args_gn(gn_args_json)

        self.out_dir.mkdir(parents=True, exist_ok=True)
        (self.out_dir / 'args.gn').write_text(
            args_gn, encoding='utf-8', newline='\n'
        )
        if secrets:
            secrets_path = self.out_dir / 'secrets.gni'
            secrets_path.write_text(
                ''.join(
                    '%s = %s\n' % (name, gn_helpers.ToGNString(value))
                    for name, value in sorted(secrets.items())
                ),
                encoding='utf-8',
                newline='\n',
            )
            # Restrictive permissions: this file carries real secret values.
            secrets_path.chmod(0o600)
        return args_gn

    def gen(self) -> int:
        """Writes the directory, then runs `gn gen` in it.

        Returns:
            `gn gen`'s exit code.

        Raises:
            BotsError: as `write()` does.
        """
        self.write()
        return self._run_gn_gen()

    def _read_gn_args(self) -> dict:
        return (
            generated_output.GeneratedBuilder(self.builder_name).read().gn_args
        )

    def _render_args_gn(self, gn_args_json: dict) -> str:
        lines = []
        if gn_args_json.get('secrets'):
            lines.append(
                'import("//%s/secrets.gni")'
                % self.out_dir.relative_to(
                    gen_paths.CHROMIUM_SRC_DIR
                ).as_posix()
            )
        if gn_args_json.get('args_file'):
            lines.append('import("%s")' % gn_args_json['args_file'])
        lines.append(gn_helpers.ToGNString(gn_args_json['gn_args']))
        return '\n'.join(lines)

    def _resolve_secrets(self, secrets: dict[str, str]) -> dict[str, str]:
        """The value of each of *secrets* in the secrets `.env` file.

        Raises:
            BotsError: the secrets `.env` file has no entry for one of them.
        """
        env = dotenv.read()
        missing = sorted(name for name in secrets if name not in env)
        if missing:
            raise generated_output.BotsError(
                'builder %r declares secret(s) %s with no matching entry in '
                '%s.'
                % (self.builder_name, ', '.join(missing), dotenv.DEFAULT_PATH)
            )
        return {name: env[name] for name in secrets}

    def _run_gn_gen(self) -> int:
        return subprocess.call(
            [
                'gn',
                'gen',
                str(self.out_dir.relative_to(gen_paths.CHROMIUM_SRC_DIR)),
                '--check',
            ],
            cwd=gen_paths.CHROMIUM_SRC_DIR,
        )


def cmd_gen(args: argparse.Namespace) -> int:
    out_dir = Path(args.out_dir).resolve() if args.out_dir else None
    return BuildDir(args.builder, out_dir).gen()


def add_subparser(subparsers) -> argparse.ArgumentParser:
    """Registers the `gen` subcommand onto `bots.py`'s subparsers."""
    gen_parser = subparsers.add_parser(
        'gen', description="Write a builder's args.gn and run `gn gen`."
    )
    generated_output.add_builder_argument(
        gen_parser, help_text='A builder name, e.g. "linux-x64-asan-brave".'
    )
    gen_parser.add_argument(
        '--out-dir',
        help='Where to write args.gn and run `gn gen` (default: '
        'out/<builder>, under the chromium checkout root).',
    )
    gen_parser.set_defaults(func=cmd_gen)
    return gen_parser
