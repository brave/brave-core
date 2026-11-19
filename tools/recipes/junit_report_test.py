#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the JUnit XML reports of `junit_report`."""

from __future__ import annotations

import base64
from pathlib import Path
import sys
import tempfile
import unittest
from xml.etree import ElementTree

sys.path.insert(0, str(Path(__file__).resolve().parent))

import junit_report as m


def _b64(text: str) -> str:
    return base64.b64encode(text.encode()).decode()


def _run(status: str, *, snippet: str = '', parts=(), ms: int = 1500) -> dict:
    return {
        'status': status,
        'elapsed_time_ms': ms,
        'output_snippet_base64': _b64(snippet),
        'result_parts': list(parts),
    }


def _part(kind: str, summary: str, file='../../brave/foo_unittest.cc', line=7):
    return {
        'type': kind,
        'file': file,
        'line': line,
        'summary_base64': _b64(summary),
    }


def _report(*iterations: dict, suite='brave_unit_tests') -> ElementTree.Element:
    summary = {'per_iteration_data': list(iterations)}
    return ElementTree.fromstring(m.render(summary, suite).encode())


def _testcase(root: ElementTree.Element, name: str) -> ElementTree.Element:
    (testcase,) = root.iterfind(f".//testcase[@name='{name}']")
    return testcase


class RenderTest(unittest.TestCase):
    def test_a_testsuite_per_test_case_with_its_counts(self):
        root = _report(
            {
                'FooTest.Passes': [_run('SUCCESS')],
                'FooTest.Fails': [_run('FAILURE')],
                'BarTest.Skips': [_run('SKIPPED')],
                'BarTest.Lost': [_run('NOTRUN')],
            }
        )
        self.assertEqual(root.get('name'), 'brave_unit_tests')
        self.assertEqual(
            {
                k: root.get(k)
                for k in ('tests', 'failures', 'errors', 'skipped')
            },
            {'tests': '4', 'failures': '1', 'errors': '1', 'skipped': '1'},
        )
        self.assertEqual(root.get('time'), '6.000')
        foo_case = root.find("testsuite[@name='FooTest']")
        self.assertEqual(foo_case.get('tests'), '2')
        self.assertEqual(foo_case.get('failures'), '1')
        passes = _testcase(root, 'Passes')
        self.assertEqual(passes.get('classname'), 'FooTest')
        self.assertEqual(passes.get('time'), '1.500')
        self.assertEqual(list(passes), [])
        self.assertIsNotNone(_testcase(root, 'Skips').find('skipped'))
        lost = _testcase(root, 'Lost').find('error')
        self.assertEqual(lost.get('type'), 'NOTRUN')

    def test_parameterised_names_keep_the_launcher_s_split(self):
        root = _report({'All/FooTest.Bar/1': [_run('SUCCESS')]})
        testcase = _testcase(root, 'Bar/1')
        self.assertEqual(testcase.get('classname'), 'All/FooTest')

    def test_a_gtest_skip_is_a_skip(self):
        root = _report(
            {'FooTest.Skips': [_run('SUCCESS', parts=[_part('skip', '')])]}
        )
        self.assertIsNotNone(_testcase(root, 'Skips').find('skipped'))

    def test_a_failure_is_its_first_fatal_assertion(self):
        root = _report(
            {
                'FooTest.Fails': [
                    _run(
                        'FAILURE',
                        snippet='the output',
                        parts=[
                            _part('failure', 'Value of: a\nGoogle Test trace:'),
                            _part('fatal_failure', 'Expected: b', line=9),
                        ],
                    )
                ]
            }
        )
        failure = _testcase(root, 'Fails').find('failure')
        self.assertEqual(
            failure.get('message'), 'foo_unittest.cc(9): Expected: b'
        )
        self.assertEqual(failure.get('type'), 'FAILURE')
        self.assertEqual(failure.text, 'the output')

    def test_a_crash_takes_its_message_from_the_output(self):
        snippet = (
            '==1==ERROR: AddressSanitizer: heap-use-after-free on address\n'
            '    #0 0x55 in Foo::Bar() ../../brave/foo.cc:12:3\n'
            'previously allocated by thread T0 here:\n'
            '    #0 0x56 in operator new(unsigned long) new.cpp:86:3\n'
            '    #1 0x57 in Foo::Make() ../../brave/foo.cc:30:10\n'
        )
        root = _report({'FooTest.Crashes': [_run('CRASH', snippet=snippet)]})
        failure = _testcase(root, 'Crashes').find('failure')
        self.assertEqual(
            failure.get('message'),
            'AddressSanitizer heap-use-after-free in Foo::Bar '
            '(brave/foo.cc:12)',
        )
        self.assertEqual(failure.get('type'), 'CRASH')

    def test_a_leak_on_exit_takes_its_message_from_the_output(self):
        snippet = (
            '==1==ERROR: LeakSanitizer: detected memory leaks\n'
            'Direct leak of 8 byte(s) in 1 object(s) allocated from:\n'
            '    #0 0x10 in __interceptor_malloc ??:0:0\n'
            '    #1 0x11 in Foo::Make() ../../brave/foo.cc:30:10\n'
            'SUMMARY: AddressSanitizer: 8 byte(s) leaked in 1 allocation(s).\n'
        )
        root = _report(
            {'FooTest.Leaks': [_run('FAILURE_ON_EXIT', snippet=snippet)]}
        )
        failure = _testcase(root, 'Leaks').find('failure')
        self.assertEqual(
            failure.get('message'),
            'LeakSanitizer direct-leak in Foo::Make (brave/foo.cc:30)',
        )

    def test_a_crash_without_a_known_message_takes_its_status(self):
        root = _report({'FooTest.Hangs': [_run('TIMEOUT', snippet='...')]})
        failure = _testcase(root, 'Hangs').find('failure')
        self.assertEqual(failure.get('message'), 'TIMEOUT')

    def test_earlier_failed_runs_are_flaky_or_rerun_failures(self):
        root = _report(
            {
                'FooTest.Flaky': [_run('FAILURE'), _run('SUCCESS')],
                'FooTest.Broken': [_run('CRASH'), _run('FAILURE')],
            }
        )
        flaky = _testcase(root, 'Flaky')
        self.assertIsNone(flaky.find('failure'))
        self.assertEqual(flaky.find('flakyFailure').get('type'), 'FAILURE')
        broken = _testcase(root, 'Broken')
        self.assertEqual(broken.find('failure').get('type'), 'FAILURE')
        self.assertEqual(broken.find('rerunFailure').get('type'), 'CRASH')
        self.assertEqual(root.get('failures'), '1')

    def test_runs_across_iterations_are_one_testcase(self):
        root = _report(
            {'FooTest.Flaky': [_run('FAILURE')]},
            {'FooTest.Flaky': [_run('SUCCESS')]},
        )
        self.assertEqual(root.get('tests'), '1')
        self.assertIsNotNone(_testcase(root, 'Flaky').find('flakyFailure'))

    def test_synthetic_tests_are_left_out(self):
        root = _report(
            {
                'GoogleTestVerification.UninstantiatedParameterizedTestSuite'
                '<FooTest>': [_run('SUCCESS')],
            }
        )
        self.assertEqual(root.get('tests'), '0')

    def test_text_xml_cannot_carry_is_replaced(self):
        root = _report(
            {
                'FooTest.Fails': [
                    _run('FAILURE', snippet='\x1b[31mred\x1b[0m ]]> <&>')
                ]
            }
        )
        failure = _testcase(root, 'Fails').find('failure')
        self.assertEqual(failure.text, '�[31mred�[0m ]]> <&>')

    def test_an_empty_summary_is_an_empty_report(self):
        root = ElementTree.fromstring(m.render({}, 'brave_unit_tests'))
        self.assertEqual(root.get('tests'), '0')


class WriteTest(unittest.TestCase):
    def test_writes_the_report_making_its_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'results' / 'brave_unit_tests.xml'
            m.write(path, {}, 'brave_unit_tests')
            self.assertTrue(
                path.read_text(encoding='utf-8').startswith('<?xml')
            )


if __name__ == '__main__':
    unittest.main()
