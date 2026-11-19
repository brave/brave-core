#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the sanitiser report parser in `sanitizer_reports`."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import sanitizer_reports as m
from sanitizer_reports import Frame, Sanitizer

USE_AFTER_FREE = """\
[ RUN      ] FooTest.Frees
=================================================================
==4242==ERROR: AddressSanitizer: heap-use-after-free on address 0x6020 at pc 0x55
READ of size 8 at 0x6020 thread T0
    #0 0x55 in brave::Foo::Bar(int) const ../../brave/foo.cc:12:3
    #1 0x56 in brave::Foo::Run() ../../brave/foo.cc:30:5
    #2 0x57 in FooTest_Frees_Test::TestBody() ../../brave/foo_unittest.cc:9:3
0x6020 is located 0 bytes inside of 16-byte region
freed by thread T0 here:
    #0 0x60 in operator delete(void*) ??:0:0
    #1 0x61 in brave::Foo::Reset() ../../brave/foo.cc:40:3
SUMMARY: AddressSanitizer: heap-use-after-free ../../brave/foo.cc:12:3 in brave::Foo::Bar(int) const
Shadow bytes around the buggy address:
==4242==ABORTING
"""

LEAKS = """\
[ RUN      ] BarTest.Leaks
[       OK ] BarTest.Leaks (5 ms)

=================================================================
==99==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 64 byte(s) in 1 object(s) allocated from:
    #0 0x10 in __interceptor_malloc ??:0:0
    #1 0x11 in operator new(unsigned long) ??:0:0
    #2 0x12 in std::__Cr::unique_ptr<brave::Bar> std::__Cr::make_unique<brave::Bar>() ../../buildtools/third_party/libc++/memory:1:1
    #3 0x13 in brave::BarFactory::Make() ../../brave/bar.cc:21:10

Direct leak of 32 byte(s) in 1 object(s) allocated from:
    #0 0x10 in __interceptor_malloc ??:0:0
    #1 0x13 in brave::BarFactory::Make() ../../brave/bar.cc:22:10

Indirect leak of 8 byte(s) in 1 object(s) allocated from:
    #0 0x10 in __interceptor_malloc ??:0:0
    #1 0x14 in brave::Bar::Bar() ../../brave/bar.cc:5:1

Direct leak of 16 byte(s) in 1 object(s) allocated from:
    #0 0x10 in __interceptor_malloc ??:0:0
    #1 0x15 in brave::Baz::Make() ../../brave/baz.cc:3:1

SUMMARY: AddressSanitizer: 120 byte(s) leaked in 4 allocation(s).
"""

INDIRECT_LEAKS = """\
==7==ERROR: LeakSanitizer: detected memory leaks

Indirect leak of 128 byte(s) in 2 object(s) allocated from:
    #0 0x10 in __interceptor_malloc ??:0:0
    #1 0x11 in __rustc::__rust_alloc build/rust/allocator/lib.rs:74:20
    #2 0x12 in <alloc::boxed::Box<skus::Task>>::new library/alloc/src/boxed.rs:1:1
    #3 0x13 in <skus::SDK<skus::Client>>::summary brave/skus/lib.rs:367:20

Indirect leak of 8 byte(s) in 1 object(s) allocated from:
    #0 0x10 in __interceptor_malloc ??:0:0
    #1 0x14 in skus::initialize_sdk brave/components/skus/lib.rs:298:15

SUMMARY: AddressSanitizer: 136 byte(s) leaked in 3 allocation(s).
"""

UNINITIALISED = """\
==31==WARNING: MemorySanitizer: use-of-uninitialized-value
    #0 0x20 in brave::Parse(std::__Cr::basic_string_view<char>) ../../brave/parse.cc:8:7
    #1 0x21 in brave::Load() ../../brave/load.cc:3:3
  Uninitialized value was created by an allocation of 'x' in the stack frame
    #0 0x22 in brave::Load() ../../brave/load.cc:2:1
SUMMARY: MemorySanitizer: use-of-uninitialized-value ../../brave/parse.cc:8:7 in brave::Parse(std::__Cr::basic_string_view<char>)
"""

OVERFLOW = """\
../../brave/math.cc:14:12: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior ../../brave/math.cc:14:12
"""

DATA_RACE = """\
==================
WARNING: ThreadSanitizer: data race (pid=1234)
  Write of size 4 at 0x7b04 by thread T1:
    #0 0x30 in brave::Counter::Increment() ../../brave/counter.cc:7:5
    #1 0x31 in brave::Worker::Run() ../../brave/worker.cc:11:3
  Previous read of size 4 at 0x7b04 by main thread:
    #0 0x32 in brave::Counter::Get() ../../brave/counter.cc:9:12
SUMMARY: ThreadSanitizer: data race ../../brave/counter.cc:7:5 in brave::Counter::Increment()
==================
"""


def _parse(output: str) -> m.RunReports:
    parser = m.ReportParser()
    for line in output.splitlines():
        parser.process_line(line)
    parser.finish()
    return parser.result


def _only_report(output: str) -> m.Report:
    reports = _parse(output).reports
    if len(reports) != 1:
        raise AssertionError(f'expected one report, got {reports}')
    return reports[0]


class ReportParserTest(unittest.TestCase):
    def test_an_address_sanitizer_error_is_about_its_first_stack(self):
        report = _only_report(USE_AFTER_FREE)
        self.assertEqual(report.sanitizer, Sanitizer.ADDRESS)
        self.assertEqual(report.kind, 'heap-use-after-free')
        self.assertEqual(
            report.summary,
            'heap-use-after-free ../../brave/foo.cc:12:3 in '
            'brave::Foo::Bar(int) const',
        )
        self.assertEqual(report.test, 'FooTest.Frees')
        self.assertEqual(
            report.frames[0], Frame('brave::Foo::Bar', 'brave/foo.cc:12')
        )
        self.assertEqual(len(report.frames), 3)
        self.assertEqual(
            report.signature,
            'AddressSanitizer: heap-use-after-free in brave::Foo::Bar < '
            'brave::Foo::Run < FooTest_Frees_Test::TestBody',
        )
        self.assertEqual(
            report.headline,
            'AddressSanitizer heap-use-after-free in brave::Foo::Bar '
            '(brave/foo.cc:12)',
        )
        self.assertTrue(report.text.startswith('==4242==ERROR'))
        self.assertTrue(report.text.endswith(report.summary))

    def test_each_direct_leak_is_a_finding_once(self):
        reports = _parse(LEAKS).reports
        self.assertEqual(
            [report.signature for report in reports],
            [
                'LeakSanitizer: direct-leak in brave::BarFactory::Make',
                'LeakSanitizer: direct-leak in brave::Baz::Make',
            ],
        )
        self.assertEqual(reports[0].kind, 'direct-leak')
        self.assertEqual(reports[0].test, 'BarTest.Leaks')
        self.assertIn('Direct leak of 64 byte(s)', reports[0].text)
        self.assertNotIn('Indirect leak', reports[0].text)

    def test_indirect_leaks_alone_are_one_finding(self):
        report = _only_report(INDIRECT_LEAKS)
        self.assertEqual(report.kind, 'indirect-leak')
        self.assertEqual(
            report.signature,
            'LeakSanitizer: indirect-leak in <skus::SDK>::summary',
        )
        self.assertIn('skus::initialize_sdk', report.text)

    def test_a_memory_sanitizer_warning(self):
        report = _only_report(UNINITIALISED)
        self.assertEqual(report.sanitizer, Sanitizer.MEMORY)
        self.assertEqual(
            report.signature,
            'MemorySanitizer: use-of-uninitialized-value in brave::Parse < '
            'brave::Load',
        )

    def test_an_undefined_behaviour_without_a_stack_names_its_line(self):
        report = _only_report(OVERFLOW)
        self.assertEqual(report.sanitizer, Sanitizer.UNDEFINED)
        self.assertEqual(report.kind, 'signed integer overflow')
        self.assertEqual(report.frames, (Frame('', 'brave/math.cc:14:12'),))
        self.assertEqual(
            report.signature,
            'UndefinedBehaviorSanitizer: signed integer overflow in '
            'brave/math.cc:14:12',
        )

    def test_a_data_race(self):
        report = _only_report(DATA_RACE)
        self.assertEqual(report.sanitizer, Sanitizer.THREAD)
        self.assertEqual(report.kind, 'data race')
        self.assertEqual(
            report.signature,
            'ThreadSanitizer: data race in brave::Counter::Increment < '
            'brave::Worker::Run',
        )

    def test_the_signature_ignores_line_numbers_and_sizes(self):
        moved = USE_AFTER_FREE.replace('foo.cc:12:3', 'foo.cc:99:1').replace(
            '0x6020', '0x7777'
        )
        self.assertEqual(
            _parse(moved).reports[0].id, _parse(USE_AFTER_FREE).reports[0].id
        )

    def test_reports_are_kept_in_order(self):
        reports = _parse(USE_AFTER_FREE + LEAKS + DATA_RACE).reports
        self.assertEqual(
            [report.sanitizer for report in reports],
            [
                Sanitizer.ADDRESS,
                Sanitizer.LEAK,
                Sanitizer.LEAK,
                Sanitizer.THREAD,
            ],
        )

    def test_a_report_cut_off_is_kept(self):
        cut_off = USE_AFTER_FREE.split('SUMMARY', maxsplit=1)[0]
        report = _only_report(cut_off)
        self.assertEqual(report.kind, 'heap-use-after-free')
        self.assertEqual(report.summary, '')

    def test_running_out_of_memory_makes_the_run_unreliable(self):
        result = _parse(
            '==1==ERROR: AddressSanitizer failed to allocate 0x1000 bytes\n'
        )
        self.assertEqual(
            result.unreliable,
            ('==1==ERROR: AddressSanitizer failed to allocate 0x1000 bytes',),
        )

    def test_output_without_reports(self):
        self.assertEqual(
            _parse('[ RUN      ] A.B\n[  PASSED  ] 1 test.\n'), m.RunReports()
        )


class FrameTest(unittest.TestCase):
    def test_parses_a_symbolised_frame(self):
        self.assertEqual(
            Frame.parse(
                '    #3 0x13 in brave::Bar::Make() ../../brave/bar.cc:21:10'
            ),
            Frame('brave::Bar::Make', 'brave/bar.cc:21'),
        )

    def test_an_unsymbolised_frame_has_no_location(self):
        self.assertEqual(
            Frame.parse('    #0 0x10 in __interceptor_malloc ??:0:0'),
            Frame('__interceptor_malloc'),
        )

    def test_a_location_without_a_line(self):
        self.assertEqual(
            Frame.parse(
                '    #1 0x5c in HandleExceptionsInMethodIfSupported<testing::Test,'
                ' void> third_party/googletest/src/gtest.cc'
            ),
            Frame(
                'HandleExceptionsInMethodIfSupported',
                'third_party/googletest/src/gtest.cc',
            ),
        )

    def test_a_module_offset_is_not_a_location(self):
        self.assertEqual(
            Frame.parse(
                '    #4 0x7f in __libc_start_main '
                '(/lib/x86_64-linux-gnu/libc.so.6+0x29d8f)'
            ),
            Frame('__libc_start_main'),
        )

    def test_a_line_that_is_not_a_frame(self):
        self.assertIsNone(Frame.parse('READ of size 8 at 0x6020 thread T0'))

    def test_function_names_drop_templates_and_return_types(self):
        cases = {
            'brave::Foo::Bar(int) const': 'brave::Foo::Bar',
            'std::__Cr::unique_ptr<Bar> std::__Cr::make_unique<Bar>()': 'std::__Cr::make_unique',
            'scoped_refptr<Foo> base::MakeRefCounted<Foo, int&>(int&)': 'base::MakeRefCounted',
            'base::OnceCallback<void ()>::Run() &&': 'base::OnceCallback::Run',
            'operator new(unsigned long)': 'operator new',
            'brave::Foo::operator()(int)': 'brave::Foo::operator()',
            '<skus::NativeClient as skus::storage::StorageClient>::get_orders': '<skus::NativeClient as skus::storage::StorageClient>::get_orders',
            '<skus::SDK<skus::Client>>::summary': '<skus::SDK>::summary',
        }
        for function, bare in cases.items():
            with self.subTest(function):
                self.assertEqual(
                    Frame.parse(f'    #0 0x1 in {function} ../../a.cc:1:1'),
                    Frame(bare, 'a.cc:1'),
                )

    def test_runtime_frames(self):
        for function in (
            '__interceptor_malloc',
            '___interceptor_realloc',
            '__asan_memcpy',
            'operator new',
            'malloc',
            '__rustc::__rust_alloc',
            '<alloc::boxed::Box<T>>::new',
            'std::__Cr::make_unique',
            'base::internal::Invoker::RunOnce',
            'base::OnceCallback::Run',
            'MakeRefCounted',
            'Invoke',
        ):
            with self.subTest(function):
                self.assertTrue(Frame(function).is_runtime)
        self.assertFalse(Frame('brave::Foo::Bar').is_runtime)


class RunReportsTest(unittest.TestCase):
    def test_round_trips_through_a_file(self):
        result = _parse(USE_AFTER_FREE + LEAKS)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'out' / 'reports.json'
            result.write(path)
            self.assertEqual(m.RunReports.read(path), result)

    def test_writes_each_report_s_signature_and_id(self):
        report = _parse(USE_AFTER_FREE).reports[0].to_json()
        self.assertEqual(report['id'], _parse(USE_AFTER_FREE).reports[0].id)
        self.assertIn(
            'heap-use-after-free in brave::Foo::Bar', report['signature']
        )


if __name__ == '__main__':
    unittest.main()
