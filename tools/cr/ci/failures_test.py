#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import failures  # noqa: E402  -- after _boot
from ci.failures import (  # noqa: E402  -- after _boot
    FailedBuild,
    FailedTest,
    Kind,
)


def _kind(**fields) -> Kind:
    return FailedBuild('Build', 'https://ci/1', 'Failed', **fields).kind


class KindTest(unittest.TestCase):
    _TEST = (FailedTest('Suite.Test', 'FAILURE'),)

    def test_machine_problems_are_infra(self):
        for log in (
            '[build] Run gn clean before switching from ninja to siso in out',
            'OSError: [Errno 28] No space left on device',
            'write siso.INFO: There is not enough space on the disk.',
            'vol/attach failed: exit status 1',
            "Volume vol-1 is in state in-use/attached and can't be attached",
        ):
            with self.subTest(log=log):
                self.assertEqual(_kind(log_tail=log), Kind.INFRA)

    def test_an_aborted_build_is_infra(self):
        self.assertEqual(_kind(aborted=True, problems=('Timeout',)), Kind.INFRA)

    def test_only_tests(self):
        self.assertEqual(_kind(tests=self._TEST), Kind.TESTS)

    def test_a_failed_step_is_a_build_failure(self):
        self.assertEqual(
            _kind(tests=self._TEST, problems=('Exit code 1',)), Kind.BUILD
        )
        self.assertEqual(_kind(log_tail='error'), Kind.BUILD)


class WithFatalLineTest(unittest.TestCase):
    _FATAL = (
        'F1108 13:30:24.969724   11708 glog.go:261] log: exiting because of '
        'error writing previous log to sinks: There is not enough space on '
        'the disk.'
    )

    def test_a_fatal_line_above_the_tail_leads_it(self):
        log = '\n'.join(
            [
                'I1108 13:30:20.1 11708 build.go:1] start',
                f'[2026-11-08T13:30:25.238Z] {self._FATAL}',
                'I1108 13:30:25.1 11708 build.go:2] spew',
                'Finished: FAILURE',
            ]
        )
        tail = failures.tail(log, lines=2)
        found = failures.with_fatal_line(log, tail)
        self.assertEqual(found, f'{self._FATAL}\n[...]\n{tail}')
        self.assertEqual(_kind(log_tail=found), Kind.INFRA)

    def test_a_tail_with_the_fatal_line_or_none_is_kept(self):
        self.assertEqual(
            failures.with_fatal_line(self._FATAL, self._FATAL), self._FATAL
        )
        self.assertEqual(failures.with_fatal_line('a\nb', 'b'), 'b')


_MOCHA_OUTPUT = '\n'.join(
    [
        '[1:1:1104/051404.248329:INFO:CONSOLE:44] "  started: Page renders", '
        'source: chrome://webui-test/mocha_adapter_simple.js (44)',
        '[1:1:1104/051404.758232:INFO:CONSOLE:58] "   failed: Page saves its '
        'pref',
        'AssertionError: expected false to be true',
        '    at assertTrue (chrome://webui-test/chai_assert.js:14:12)',
        '    at Context.<anonymous> (chrome://webui-test/page_test.js:107:9)", '
        'source: chrome://webui-test/mocha_adapter_simple.js (58)',
        '../../chrome/test/base/web_ui_mocha_browser_test.cc:222: Failure',
    ]
)


class MochaFailuresTest(unittest.TestCase):
    def test_quotes_the_failed_test_and_its_error(self):
        self.assertEqual(
            failures.mocha_failures(_MOCHA_OUTPUT),
            (
                'Page saves its pref\n'
                'AssertionError: expected false to be true\n'
                '    at assertTrue (chrome://webui-test/chai_assert.js:14:12)\n'
                '    at Context.<anonymous> '
                '(chrome://webui-test/page_test.js:107:9)',
            ),
        )

    def test_a_retry_is_quoted_once(self):
        output = f'{_MOCHA_OUTPUT}\n[ RUN      ] Retry\n{_MOCHA_OUTPUT}'
        self.assertEqual(len(failures.mocha_failures(output)), 1)

    def test_a_long_stack_is_cut(self):
        output = '\n'.join(
            [
                '[1:1:0:INFO:CONSOLE:58] "failed: Page t',
                *[f'    at f{i} (x.js:1:1)' for i in range(20)],
            ]
        )
        found = failures.mocha_failures(output)
        self.assertEqual(len(found), 1)
        failure = found[0]
        self.assertEqual(len(failure.splitlines()), 8)

    def test_other_output_has_none(self):
        self.assertEqual(
            failures.mocha_failures('Expected: true\nfailed: x'), ()
        )


if __name__ == '__main__':
    unittest.main()
