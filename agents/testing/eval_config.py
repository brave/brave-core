# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test metadata for the skill-eval harness.

Ported from chromium //agents/testing/eval_config.py. Changes vs. upstream:
  * Drop the `constants` import; the repo root is derived locally.
  * Filtering is by the test's `tags` metadata (the harness `--tag-filter`
    selects tags like `stable`), in addition to upstream's path-glob filter.

A test's tuning knobs live in the `metadata` of the FIRST test in its
`*.promptfoo.yaml`:
    metadata:
      runs_per_test: 3        # run the (nondeterministic) agent N times
      pass_k_threshold: 2     # pass if >= k of N runs pass
      tags: ['stable']        # selected by run_evals.py --tag-filter
"""

import dataclasses
import fnmatch
import logging
import pathlib

import yaml

# .../src/brave/agents/testing/eval_config.py -> parents[2] == .../src/brave
BRAVE_SRC = pathlib.Path(__file__).resolve().parents[2]


@dataclasses.dataclass
class TestConfig:
    """Configuration for a single eval test.

    Note that `runs_per_test` and `pass_k_threshold` may not match the values
    in `test_file` unless this object is constructed with `from_file`.
    """
    test_file: pathlib.Path
    description: str = ""
    owner: str = None
    runs_per_test: int = 1
    pass_k_threshold: int = 1
    precompile_targets: list = dataclasses.field(default_factory=list)
    tags: list = dataclasses.field(default_factory=list)

    def __lt__(self, other: 'TestConfig') -> bool:
        return self.test_file < other.test_file

    def validate(self):
        if not isinstance(self.runs_per_test, int) or self.runs_per_test <= 0:
            raise ValueError(
                f'runs_per_test in {self.test_file} must be a positive integer.'
            )
        if (not isinstance(self.pass_k_threshold, int)
                or self.pass_k_threshold < 0):
            raise ValueError(
                f'pass_k_threshold in {self.test_file} must be a non-negative '
                'integer.')
        if self.runs_per_test < self.pass_k_threshold:
            raise ValueError(f'runs_per_test in {self.test_file} must be >= '
                             'pass_k_threshold.')
        if not isinstance(self.tags, list) or not all(
                isinstance(t, str) for t in self.tags):
            raise ValueError(
                f'tags in {self.test_file} must be a list of strings.')

    @property
    def src_relative_test_file(self) -> pathlib.Path:
        try:
            return self.test_file.relative_to(BRAVE_SRC)
        except ValueError:
            return self.test_file

    @classmethod
    def from_file(cls, test_file: pathlib.Path) -> 'TestConfig':
        """Reads the test config from the test file."""
        try:
            with open(test_file, 'r', encoding='utf-8') as f:
                config = yaml.safe_load(f)
        except FileNotFoundError as e:
            raise ValueError(f'Test config file not found: {test_file}') from e
        except yaml.YAMLError as e:
            raise ValueError(f'Error parsing YAML file: {test_file}') from e

        if config is None:
            raise ValueError(
                f'Test config file must not be empty: {test_file}')

        if 'tests' not in config:
            raise ValueError(f'Test config file must have a "tests" key: '
                             f'{test_file}')

        if not config['tests']:
            raise ValueError(f'"tests" list in {test_file} must not be empty.')

        runs_per_test = 1
        pass_k_threshold = 1
        precompile_targets = []
        tags = []
        if len(config['tests']) > 1:
            logging.warning(
                'Test settings can only be specified on the first test in a '
                'promptfoo config. Settings on other tests will be ignored.')

        test = config['tests'][0]
        metadata = test.get('metadata') if isinstance(test, dict) else None
        if metadata:
            runs_per_test = metadata.get('runs_per_test', 1)
            pass_k_threshold = metadata.get('pass_k_threshold', runs_per_test)
            precompile_targets = metadata.get('precompile_targets', [])
            tags = metadata.get('tags', [])
        owner = config.get('owner')
        description = config.get('description', '')

        instance = cls(test_file=test_file,
                       description=description,
                       runs_per_test=runs_per_test,
                       pass_k_threshold=pass_k_threshold,
                       precompile_targets=precompile_targets,
                       owner=owner,
                       tags=tags)
        instance.validate()
        return instance

    def matches_path_filter(self, filters: list) -> bool:
        """True if the test file path matches any glob in `filters`."""
        relative_path = str(self.src_relative_test_file)
        return any(fnmatch.fnmatch(relative_path, f) for f in filters)

    def matches_tags(self, wanted_tags: list) -> bool:
        """True if the test carries any of `wanted_tags` (empty = match all)."""
        if not wanted_tags:
            return True
        return any(t in self.tags for t in wanted_tags)
