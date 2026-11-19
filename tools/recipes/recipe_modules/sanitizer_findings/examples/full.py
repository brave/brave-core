# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `sanitizer_findings` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import sanitizer_findings
from recipe_test_api import RecipeTestApi
from sanitizer_reports import ReportParser, RunReports

_USE_AFTER_FREE = """\
[ RUN      ] FooTest.Frees
==1==ERROR: AddressSanitizer: heap-use-after-free on address 0x6020
    #0 0x55 in brave::Foo::Bar() ../../brave/foo.cc:12:3
SUMMARY: AddressSanitizer: heap-use-after-free ../../brave/foo.cc:12:3
"""

_LEAK = """\
[ RUN      ] BarTest.Leaks
==2==ERROR: LeakSanitizer: detected memory leaks
Direct leak of 8 byte(s) in 1 object(s) allocated from:
    #0 0x10 in __interceptor_malloc ??:0:0
    #1 0x11 in brave::Bar::Make() ../../brave/bar.cc:3:1
SUMMARY: AddressSanitizer: 8 byte(s) leaked in 1 allocation(s).
"""

_OUT_OF_MEMORY = '==3==ERROR: AddressSanitizer failed to allocate 0x10 bytes\n'


@dataclass
class DEPS(RecipeScriptApi):
    sanitizer_findings: sanitizer_findings.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    pass


def _parse(output: str) -> RunReports:
    parser = ReportParser()
    for line in output.splitlines():
        parser.process_line(line)
    parser.finish()
    return parser.result


def RunSteps(api: DEPS):
    api.sanitizer_findings.report(
        {
            'brave_unit_tests': _parse(_USE_AFTER_FREE + _LEAK),
            'brave_browser_tests': _parse(_USE_AFTER_FREE),
            'brave_components_unittests': _parse(_OUT_OF_MEMORY),
        }
    )


def _one_finding_per_bug(check, value: dict) -> bool:
    return check(
        [finding['headline'] for finding in value['findings']]
        == [
            'AddressSanitizer heap-use-after-free in brave::Foo::Bar '
            '(brave/foo.cc:12)',
            'LeakSanitizer direct-leak in brave::Bar::Make (brave/bar.cc:3)',
        ]
    )


def _suites_named(check, value: dict) -> bool:
    use_after_free = value['findings'][0]
    return check(
        use_after_free['suites'] == ['brave_unit_tests', 'brave_browser_tests']
        and use_after_free['tests'] == ['FooTest.Frees']
        and use_after_free['occurrences'] == 2
        and value['unreliable_suites'] == ['brave_components_unittests']
    )


def GenTests(api: TEST_DEPS):
    # The same bug in two suites is one finding; a suite whose sanitiser ran
    # out of memory is named as unreliable.
    yield api.test(
        'basic',
        api.post_process(
            post_process.PropertyMatchesCallable,
            'sanitizer_findings',
            _one_finding_per_bug,
        ),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'sanitizer_findings',
            _suites_named,
        ),
    )
