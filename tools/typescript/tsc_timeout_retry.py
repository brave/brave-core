# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Workaround for rare hangs in the TypeScript compiler (tsc) during GN build
# actions. tools/typescript/ts_library.py invokes tsc via
# subprocess.Popen(...).communicate(), and devtools-frontend's
# run_with_restat.py via subprocess.call(), neither with a timeout, so a hung
# tsc blocks the build indefinitely.
#
# patch_subprocess_with_timeout_retry() is used as a context manager in
# chromium_src overrides of those scripts. While active, it replaces
# subprocess.Popen with a wrapper that applies only to tsc invocations (tsc
# itself, or node running it). Other subprocess calls are unaffected.
#
# On timeout the hung process is killed and tsc is re-run from scratch. The
# timeout doubles after each failed attempt. Defaults and retry count are
# configurable via CHROMIUM_TSC_TIMEOUT_SEC and CHROMIUM_TSC_MAX_RETRIES.

import contextlib
import logging
import os
import subprocess
import sys

DEFAULT_TSC_TIMEOUT_SEC = int(os.environ.get('CHROMIUM_TSC_TIMEOUT_SEC', '90'))
DEFAULT_TSC_MAX_RETRIES = int(os.environ.get('CHROMIUM_TSC_MAX_RETRIES', '2'))


class _RetryPopen:
    """Wraps subprocess.Popen with timeout and retry for hung tsc
    invocations."""

    def __init__(self, original_popen, popen_args, popen_kwargs, timeout_sec,
                 max_retries):
        self._original_popen = original_popen
        self._popen_args = popen_args
        self._popen_kwargs = popen_kwargs
        self._timeout_sec = timeout_sec
        self._max_retries = max_retries
        self._cmd_repr = self._command_repr(popen_args, popen_kwargs)
        self._process = original_popen(*popen_args, **popen_kwargs)

    def _recreate_process(self):
        self._process = self._original_popen(*self._popen_args,
                                             **self._popen_kwargs)

    def _retry_until_done(self, attempt):
        effective_timeout = self._timeout_sec
        max_attempts = self._max_retries + 1
        for attempt_number in range(1, max_attempts + 1):
            try:
                return attempt(effective_timeout)
            except subprocess.TimeoutExpired as exc:
                self._process.kill()
                try:
                    self._process.communicate()
                except Exception:
                    pass
                if attempt_number >= max_attempts:
                    raise RuntimeError(
                        f'tsc timed out after {effective_timeout}s '
                        f'({max_attempts} attempts): {self._cmd_repr}'
                    ) from exc
                logging.warning(
                    'tsc timed out after %ss (attempt %d/%d), retrying: %s',
                    effective_timeout, attempt_number, max_attempts,
                    self._cmd_repr)
                effective_timeout *= 2
                self._recreate_process()

        raise AssertionError('unreachable')

    def communicate(self, *args, **kwargs):
        if 'timeout' in kwargs:
            return self._process.communicate(*args, **kwargs)

        return self._retry_until_done(
            lambda timeout: self._process.communicate(
                *args, **dict(kwargs, timeout=timeout)))

    # subprocess.call(), which devtools-frontend's run_with_restat.py uses,
    # waits on the process rather than communicating with it.
    def wait(self, *args, **kwargs):
        if args or kwargs.get('timeout') is not None:
            return self._process.wait(*args, **kwargs)

        return self._retry_until_done(
            lambda timeout: self._process.wait(timeout=timeout))

    def __getattr__(self, name):
        return getattr(self._process, name)

    def __enter__(self):
        self._process.__enter__()
        return self

    def __exit__(self, *args):
        return self._process.__exit__(*args)

    @staticmethod
    def _command_from_popen_args(args, kwargs):
        if args:
            return args[0]
        return kwargs.get('args', [])

    @staticmethod
    def _is_tsc_path(part):
        # Both path separators are handled, so this doesn't depend on the host.
        basename = str(part).replace('\\', '/').rsplit('/', 1)[-1]
        return os.path.splitext(basename)[0] == 'tsc'

    @staticmethod
    def _is_tsc_command(args, kwargs):
        cmd = _RetryPopen._command_from_popen_args(args, kwargs)
        if isinstance(cmd, str):
            cmd = [cmd]
        # Either the tsc binary invoked directly, or node running tsc.
        return any(_RetryPopen._is_tsc_path(part) for part in cmd[:2])

    @staticmethod
    def _command_repr(args, kwargs):
        cmd = _RetryPopen._command_from_popen_args(args, kwargs)
        if isinstance(cmd, str):
            return cmd
        return ' '.join(cmd)


def _make_popen(original_popen):

    def popen(*args, **kwargs):
        if _RetryPopen._is_tsc_command(args, kwargs):
            return _RetryPopen(original_popen, args, kwargs,
                               DEFAULT_TSC_TIMEOUT_SEC,
                               DEFAULT_TSC_MAX_RETRIES)
        return original_popen(*args, **kwargs)

    return popen


@contextlib.contextmanager
def patch_subprocess_with_timeout_retry():
    original_popen = subprocess.Popen
    subprocess.Popen = _make_popen(original_popen)
    try:
        yield
    finally:
        subprocess.Popen = original_popen
