# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `isolate` module API."""

from __future__ import annotations

from collections.abc import Sequence
from dataclasses import dataclass
from typing import TYPE_CHECKING

from PB.recipe_modules.brave.isolate.properties import InputProperties
from recipe_api import RecipeApi

if TYPE_CHECKING:
    from pathlib import Path

    import config_types
    from recipe_modules import isolate
    from step_data import StepData

# Brave's suites, on top of Chromium's.
ISOLATE_MAP = 'brave/infra/config/generated/testing/gn_isolate_map.pyl'


@dataclass(frozen=True)
class Isolate:
    """A suite archived in CAS, and how to run it from there."""

    # The tree's digest, as `<hash>/<size>`.
    digest: str
    # The command the suite runs with.
    command: tuple[str, ...]
    # The directory in the tree the command runs in.
    cwd: str

    def to_json(self) -> dict:
        return {
            'digest': self.digest,
            'command': list(self.command),
            'cwd': self.cwd,
        }

    @classmethod
    def from_json(cls, data: dict) -> Isolate:
        return cls(
            digest=data['digest'],
            command=tuple(data['command']),
            cwd=data['cwd'],
        )


class IsolateApi(RecipeApi):
    """Archives test suites, each with the files `mb isolate` lists for it,
    so another machine can run them with no checkout or build."""

    m: isolate.DEPS

    def __init__(self, properties: InputProperties) -> None:
        super().__init__()
        self._enabled = properties.enabled

    @property
    def enabled(self) -> bool:
        """Whether the builder archives its test suites."""
        return self._enabled

    def isolate_tests(
        self,
        src: str | Path | config_types.Path,
        build_dir: str,
        targets: Sequence[str],
    ) -> dict[str, Isolate]:
        """Archives each of *targets*, built in *build_dir*.

        Args:
            src: The Chromium checkout.
            build_dir: The build directory, relative to *src*.
            targets: The suites to archive.

        Returns:
            Each suite's isolate, by its name. The `isolates` output property
            holds the same.
        """
        isolates = {}
        with self.m.step.nest('isolate tests'):
            for target in targets:
                with self.m.step.nest(target):
                    isolates[target] = self._isolate(src, build_dir, target)
            self.m.step('isolates', None).presentation.properties[
                'isolates'
            ] = {name: i.to_json() for name, i in isolates.items()}
        return isolates

    def run(
        self, step_name: str, isolate: Isolate, args: Sequence[str] = ()
    ) -> StepData:
        """Downloads *isolate*'s tree into a new directory and runs its
        command there, with *args* after it.

        Args:
            step_name: The step the download and the run nest under.
            isolate: The suite to run.
            args: Arguments to add to its command.

        Returns:
            The run's step, which does not raise when the command fails.
        """
        with self.m.step.nest(step_name):
            tree = self.m.path.mkdtemp('isolate')
            self.m.cas.download('download', isolate.digest, tree)
            # Commands start with `vpython3`, which depot_tools puts on PATH.
            self.m.depot_tools.ensure_on_path()
            with self.m.context(cwd=tree / isolate.cwd):
                return self.m.step(
                    'run', [*isolate.command, *args], check=False
                )

    def _isolate(
        self, src: str | Path | config_types.Path, build_dir: str, target: str
    ) -> Isolate:
        with self.m.context(cwd=src):
            self.m.step(
                'mb isolate',
                [
                    self.m.depot_tools.vpython3(),
                    src / 'tools/mb/mb.py',
                    'isolate',
                    '--no-build',
                    build_dir,
                    target,
                    '--isolate-map-file',
                    src / ISOLATE_MAP,
                ],
            )
        variables = self.m.file.read_json(
            'read isolate',
            src / build_dir / f'{target}.isolate',
            test_data=self.test_api.isolate_file(target),
        )['variables']
        digest = self.m.cas.archive(
            'archive', src, variables['files'], cwd=src / build_dir
        )
        return Isolate(
            digest=digest, command=tuple(variables['command']), cwd=build_dir
        )
