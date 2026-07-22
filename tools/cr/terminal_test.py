#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest

from terminal import terminal


class TerminalStdinTest(unittest.TestCase):
    """`terminal.run(stdin=...)` must round-trip bytes untouched.

    The stdin path runs the child's pipes in binary mode, so no layer -- OS or
    Python -- translates newlines in either direction. `plaster.run_ast_grep`
    maps ast-grep's byte offsets back onto the source it sent, so any rewrite
    (e.g. text mode adding '\\r' on Windows) would shift every offset and
    corrupt the resulting edits. These tests guard the byte-exact contract on
    every platform.
    """

    def test_str_stdin_round_trips_verbatim(self):
        source = 'line1\nline2\nline3\n'
        result = terminal.run(['cat'], stdin=source)
        self.assertIsInstance(result.stdout, str)
        self.assertEqual(result.stdout, source)

    def test_crlf_is_preserved_verbatim(self):
        # CRLF in must be CRLF out -- never folded to LF -- and LF-only input
        # must never gain a '\r'. Cover CRLF, a lone CR, and LF together.
        for source in ('a\r\nb\r\nc\r\n', 'x\ry\r\nz\n', 'a\nb\n'):
            with self.subTest(source=source):
                result = terminal.run(['cat'], stdin=source)
                self.assertEqual(result.stdout, source)

    def test_no_byte_count_change_on_stdin(self):
        # The child sees exactly the bytes we passed, so its byte count matches
        # the source's, regardless of the host platform's line separator.
        source = 'a\nb\nc\n'
        result = terminal.run(['wc', '-c'], stdin=source)
        self.assertEqual(result.stdout.strip(),
                         str(len(source.encode('utf-8'))))

    def test_bytes_stdin_is_sent_as_is(self):
        result = terminal.run(['cat'], stdin=b'raw\r\nbytes\n')
        self.assertEqual(result.stdout, 'raw\r\nbytes\n')

    def test_stdin_rejected_in_interactive_mode(self):
        with self.assertRaises(ValueError):
            terminal.run(['cat'], interactive=True, stdin='data')


if __name__ == '__main__':
    unittest.main()
