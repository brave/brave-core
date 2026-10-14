#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for polymer_tags.py."""

from __future__ import annotations

import unittest

from polymer_tags import (
    BraveRegistrations,
    Registration,
    Result,
    UpstreamElements,
)

_SOURCE = """\
import {RegisterStyleOverride, RegisterPolymerTemplateModifications}
    from 'chrome://resources/brave/polymer_overriding.js'

RegisterStyleOverride('all-sites', html`<style>
  :host { --x: 'not-a-tag'; }
</style>`)

RegisterPolymerTemplateModifications({
  'site-details': (templateContent) => {
    templateContent.querySelector('other-tag')
    console.log('no-tag-here')
  },
  "site-list": () => {},
})

RegisterPolymerComponentToIgnore('add-site-dialog')
somethingElse('not-registered')
"""


class BraveRegistrationsTest(unittest.TestCase):
    """Real ast-grep runs."""

    def test_call_arguments_and_map_keys(self) -> None:
        self.assertEqual(
            [
                Registration('all-sites', 'RegisterStyleOverride', 'br/x.ts:4'),
                Registration(
                    'site-details',
                    'RegisterPolymerTemplateModifications',
                    'br/x.ts:9',
                ),
                Registration(
                    'site-list',
                    'RegisterPolymerTemplateModifications',
                    'br/x.ts:13',
                ),
                Registration(
                    'add-site-dialog',
                    'RegisterPolymerComponentToIgnore',
                    'br/x.ts:16',
                ),
            ],
            sorted(
                BraveRegistrations.parse(_SOURCE, 'br/x.ts'),
                key=lambda r: int(r.where.split(':')[1]),
            ),
        )


class FakeRepo:
    """A repository of `{path: (tag its element returns, text)}` at a tag."""

    def __init__(self, files: dict[str, tuple[str, str]]) -> None:
        self._files = files

    def run_git(self, *args: str) -> str:
        if args[0] == 'grep':
            return ''.join(
                f'{args[3]}:{path}:    return \'{tag}\';\n'
                for path, (tag, _) in self._files.items()
            )
        _, spec = args
        return self._files[spec.split(':', 1)[1]][1]


class UpstreamElementsTest(unittest.TestCase):
    def test_bases(self) -> None:
        elements = UpstreamElements(
            FakeRepo(
                {
                    'a/p.ts': ('p-el', 'class P extends PolymerElement {}'),
                    'a/l.ts': ('l-el', 'class L extends CrLitElement {}'),
                    'a/m.ts': ('m-el', 'PolymerElement CrLitElement'),
                    'a/p_test.ts': ('t-el', 'PolymerElement'),
                }
            ),
            'tag',
        )
        self.assertEqual('a/p.ts', elements.file('p-el'))
        self.assertEqual('polymer', elements.base('p-el'))
        self.assertEqual('lit', elements.base('l-el'))
        self.assertEqual('mixed', elements.base('m-el'))
        self.assertEqual('missing', elements.base('t-el'))


class ResultTest(unittest.TestCase):
    def test_buckets(self) -> None:
        registration = Registration('x-y', 'RegisterStyleOverride', 'br/x.ts:1')
        cases = {
            ('polymer', 'polymer'): 'fine',
            ('polymer', 'lit'): 'broke',
            ('polymer', 'missing'): 'broke',
            ('lit', 'lit'): 'dead',
            ('lit', 'polymer'): 'review',
            ('mixed', 'polymer'): 'review',
        }
        for (old, new), bucket in cases.items():
            self.assertEqual(
                bucket,
                Result(registration, 'a.ts', old, new).bucket,
                (old, new),
            )
        self.assertEqual(
            'review', Result(registration, None, 'missing', 'missing').bucket
        )


if __name__ == '__main__':
    unittest.main()
