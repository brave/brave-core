#!/usr/bin/env python

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import unittest
import os

from lib.helpers import *
from mock import call, MagicMock

dirname = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(dirname, '..'))


class RetryFunc():
    def __init__(self):
        self.ran = 0
        self.calls = []
        self.err = UserWarning

    def succeed(self, count):
        self.ran = self.ran + 1
        self.calls.append(count)

    def fail(self, count):
        self.ran = self.ran + 1
        self.calls.append(count)
        raise self.err


class TestRetryFunc(unittest.TestCase):
    def setUp(self):
        self.retry_func = RetryFunc()
        self.catch_func = RetryFunc()
        self.example_method = MagicMock()
        self.throw_exception = False

    def run_logic(self):
        self.example_method('run_logic1')
        if self.throw_exception:
            self.throw_exception = False
            raise UserWarning('error')
        self.example_method('run_logic2')

    def handler_logic(self):
        self.example_method('handler_logic')

    def exception_caught(self):
        self.example_method('exception_handler1')
        retry_func(
            lambda ran: self.handler_logic(),
            catch_func=lambda ran: self.exception_caught(),
            catch=UserWarning, retries=3
        )
        self.example_method('exception_handler2')

    def test_passes_retry_count(self):
        self.assertRaises(
            self.retry_func.err,
            retry_func,
            self.retry_func.fail,
            catch=UserWarning, retries=3
        )
        self.assertEqual(self.retry_func.calls, [0, 1, 2, 3])

    def test_retries_on_fail(self):
        self.assertRaises(
            self.retry_func.err,
            retry_func,
            self.retry_func.fail,
            catch=UserWarning, retries=3
        )
        self.assertEqual(self.retry_func.ran, 4)

    def test_run_catch_func_on_fail(self):
        self.assertRaises(
            self.retry_func.err,
            retry_func,
            self.retry_func.fail,
            catch_func=self.catch_func.succeed,
            catch=UserWarning, retries=3
        )
        self.assertEqual(self.catch_func.ran, 4)

    def test_no_retry_on_success(self):
        retry_func(
            self.retry_func.succeed,
            catch=UserWarning, retries=3
        )
        self.assertEqual(self.retry_func.ran, 1)

    def test_no_run_catch_func_on_success(self):
        retry_func(
            self.retry_func.succeed,
            catch_func=self.catch_func.succeed,
            catch=UserWarning, retries=3
        )
        self.assertEqual(self.catch_func.ran, 0)

    def test_runs_logic_and_handler_in_proper_order(self):
        self.throw_exception = True
        retry_func(
            lambda ran: self.run_logic(),
            catch_func=lambda ran: self.exception_caught(),
            catch=UserWarning, retries=1
        )
        expected = [call('run_logic1'),
                    call('exception_handler1'),
                    call('handler_logic'),
                    call('exception_handler2'),
                    call('run_logic1'),
                    call('run_logic2')]
        self.assertEqual(expected, self.example_method.mock_calls)


if __name__ == '__main__':
    print unittest.main()
