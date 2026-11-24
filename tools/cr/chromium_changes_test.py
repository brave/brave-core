#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest

import chromium_changes

_A = 'a' * 40
_B = 'b' * 40
_UPSTREAM_ID = 'I' + 'c' * 40


def _quote(commit: str, message: str, indent: str = '    ') -> str:
    lines = '\n'.join(
        indent + line if line else '' for line in message.split('\n')
    )
    return (
        f'commit {commit}\n'
        'Author: Someone <someone@chromium.org>\n'
        'Date:   Mon Nov 9 10:06:38 2026 -0800\n'
        f'\n{lines}'
    )


def _message(*quotes: str, links: tuple[str, ...] = (_A,)) -> str:
    urls = '\n'.join(
        f'https://chromium.googlesource.com/chromium/src/+/{c}' for c in links
    )
    return (
        'Fix\n\nWhy.\n\nChromium changes:\n'
        + urls
        + '\n\n'
        + '\n\n'.join(quotes)
    )


_UPSTREAM = f'Upstream change\n\nIts body.\n\nBug: 1\nChange-Id: {_UPSTREAM_ID}'


class ProblemTest(unittest.TestCase):
    def test_no_section(self):
        self.assertIsNone(chromium_changes.problem('Fix\n\nWhy.'))

    def test_as_the_hook_writes_it(self):
        message = _message(
            _quote(_A, _UPSTREAM), _quote(_B, 'Other'), links=(_A, _B)
        )
        self.assertIsNone(chromium_changes.problem(message))
        self.assertIsNone(
            chromium_changes.problem(f'{message}\n\nChange-Id: I{"d" * 40}')
        )

    def test_unindented_quote(self):
        problem = chromium_changes.problem(_message(_quote(_A, _UPSTREAM, '')))
        self.assertIn(
            f'culprit {_A} is quoted without its indentation', problem
        )
        self.assertIn('git cr commit --culprit', problem)

    def test_unindented_second_quote(self):
        problem = chromium_changes.problem(
            _message(
                _quote(_A, 'Indented'),
                _quote(_B, _UPSTREAM, ''),
                links=(_A, _B),
            )
        )
        self.assertIn(f'culprit {_B}', problem)

    def test_no_links(self):
        problem = chromium_changes.problem(
            f'Fix\n\nChromium changes:\n\n{_quote(_A, _UPSTREAM)}'
        )
        self.assertIn("not followed by the culprits' links", problem)

    def test_links_without_quotes(self):
        problem = chromium_changes.problem(
            'Fix\n\nChromium changes:\n'
            f'https://chromium.googlesource.com/chromium/src/+/{_A}\n\n'
            'Upstream change'
        )
        self.assertIn('does not quote its culprits', problem)


class FixedTest(unittest.TestCase):
    def test_leaves_the_hooks_alone(self):
        message = _message(_quote(_A, _UPSTREAM))
        self.assertEqual(chromium_changes.fixed(message), message)

    def test_indents_an_unindented_quote(self):
        self.assertEqual(
            chromium_changes.fixed(_message(_quote(_A, _UPSTREAM, ''))),
            _message(_quote(_A, _UPSTREAM)),
        )

    def test_indents_each_quote_up_to_the_next(self):
        self.assertEqual(
            chromium_changes.fixed(
                _message(
                    _quote(_A, _UPSTREAM, ''),
                    _quote(_B, _UPSTREAM, ''),
                    links=(_A, _B),
                )
            ),
            _message(
                _quote(_A, _UPSTREAM), _quote(_B, _UPSTREAM), links=(_A, _B)
            ),
        )
        self.assertIsNone(
            chromium_changes.problem(
                chromium_changes.fixed(
                    _message(_quote(_A, _UPSTREAM, ''), _quote(_B, 'Other'))
                )
            )
        )


if __name__ == '__main__':
    unittest.main()
