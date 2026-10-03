# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Inline overrides for local_device_gtest_run.py."""

import os
import sys
import threading


def _is_teamcity_service_message(line):
    return (
        os.environ.get('TEAMCITY_VERSION') is not None
        and line.startswith('##teamcity[')
    )


class _TeamcityServiceMessageWatcher:
    """Relays service messages from an Android gtest stdout file."""

    def __init__(self, device, path, as_root):
        self._device = device
        self._path = path
        self._as_root = as_root
        self._offset = 0
        self._partial_line = ''
        self._stop_event = threading.Event()
        self._thread = threading.Thread(
            target=self._run,
            name='teamcity-service-message-watcher',
            daemon=True,
        )

    def __enter__(self):
        self._thread.start()
        return self

    def __exit__(self, _exc_type, _exc_value, _traceback):
        self._stop_event.set()
        self._thread.join(timeout=5)

    def _run(self):
        while not self._stop_event.wait(0.5):
            self._read_new_messages()
        self._read_new_messages()

    def _read_new_messages(self):
        try:
            content = self._device.ReadFile(
                self._path, as_root=self._as_root
            )
        # Device output streaming is best-effort. The normal result collection
        # below remains responsible for reporting device and read failures.
        # pylint: disable-next=broad-exception-caught
        except Exception:
            return

        if len(content) < self._offset:
            self._offset = 0
            self._partial_line = ''

        new_content = content[self._offset :]
        self._offset = len(content)
        lines = (self._partial_line + new_content).splitlines(keepends=True)
        self._partial_line = ''

        for line in lines:
            if not line.endswith(('\n', '\r')):
                self._partial_line = line
                continue

            line = line.rstrip('\r\n')
            if _is_teamcity_service_message(line):
                sys.stdout.write(f'{line}\n')
                sys.stdout.flush()


def _create_teamcity_service_message_watcher(device, path, as_root):
    if os.environ.get('TEAMCITY_VERSION') is None:
        return _NullContextManager()
    if as_root:
        path = device.ResolveSpecialPath(path)
    return _TeamcityServiceMessageWatcher(device, path, as_root)
