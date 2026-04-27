#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import io
import json
import os
import platform
import socket
import tempfile
import unittest
from unittest.mock import MagicMock, patch

from vscode import (VsCodeIpcConnection, _NamedPipeSocket,
                    _PosixVsCodeIpcConnection, _WinVsCodeIpcConnection)


class PosixVsCodeIpcConnectionTest(unittest.TestCase):
    """Tests for _PosixVsCodeIpcConnection."""

    def setUp(self):
        """Creates a temporary directory and derives a socket path from it."""
        self._tmp_dir = tempfile.TemporaryDirectory()
        self.sock_path = os.path.join(self._tmp_dir.name, 'vscode.sock')
        self.addCleanup(self._tmp_dir.cleanup)

    def test_init_reads_socket_path_from_env(self):
        """_socket_path is set from VSCODE_IPC_HOOK_CLI."""
        with patch.dict(os.environ, {'VSCODE_IPC_HOOK_CLI': self.sock_path}):
            conn = _PosixVsCodeIpcConnection()
            self.assertEqual(conn._socket_path, self.sock_path)

    def test_init_empty_socket_path_when_env_absent(self):
        """_socket_path is empty when VSCODE_IPC_HOOK_CLI is not set."""
        with patch.dict(os.environ, {}):
            os.environ.pop('VSCODE_IPC_HOOK_CLI', None)
            conn = _PosixVsCodeIpcConnection()
            self.assertEqual(conn._socket_path, '')

    def test_connect_creates_unix_socket(self):
        """connect() opens an AF_UNIX socket and connects to _socket_path."""
        with patch.dict(os.environ, {'VSCODE_IPC_HOOK_CLI': self.sock_path}):
            conn = _PosixVsCodeIpcConnection()
            with patch('vscode.socket.socket') as mock_socket_cls:
                mock_sock = MagicMock()
                mock_socket_cls.return_value = mock_sock
                conn.connect()
                mock_socket_cls.assert_called_once_with(
                    socket.AF_UNIX, socket.SOCK_STREAM)
                mock_sock.connect.assert_called_once_with(self.sock_path)

    def test_open_file_skips_when_no_socket_path(self):
        """open_file does nothing when VSCODE_IPC_HOOK_CLI is not set."""
        with patch.dict(os.environ, {}):
            os.environ.pop('VSCODE_IPC_HOOK_CLI', None)
            conn = _PosixVsCodeIpcConnection()
            with patch.object(conn, 'request') as mock_request:
                conn.open_file(['/some/file.cc'])
                mock_request.assert_not_called()

    def test_open_file_skips_when_files_empty(self):
        """open_file does nothing when the files list is empty."""
        with patch.dict(os.environ, {'VSCODE_IPC_HOOK_CLI': self.sock_path}):
            conn = _PosixVsCodeIpcConnection()
            with patch.object(conn, 'request') as mock_request:
                conn.open_file([])
                mock_request.assert_not_called()

    def test_open_file_sends_correct_request(self):
        """open_file sends POST / with the correct JSON body and headers."""
        with patch.dict(os.environ, {'VSCODE_IPC_HOOK_CLI': self.sock_path}):
            conn = _PosixVsCodeIpcConnection()
            mock_response = MagicMock()
            mock_response.status = 200
            mock_response.reason = 'OK'
            mock_response.read.return_value = b''
            with patch.object(conn, 'request') as mock_request, \
                 patch.object(conn, 'getresponse', return_value=mock_response):
                conn.open_file(['/path/to/file.cc'])
                mock_request.assert_called_once()
                method, path, body, headers = mock_request.call_args[0]
                self.assertEqual(method, 'POST')
                self.assertEqual(path, '/')
                payload = json.loads(body)
                self.assertEqual(payload['type'], 'open')
                self.assertEqual(payload['fileURIs'],
                                 ['file:///path/to/file.cc'])
                self.assertTrue(payload['forceReuseWindow'])
                self.assertEqual(headers['Content-Type'], 'application/json')

    def test_open_file_logs_error_on_failure(self):
        """open_file logs a warning and does not raise on connection failure."""
        with patch.dict(os.environ, {'VSCODE_IPC_HOOK_CLI': self.sock_path}):
            conn = _PosixVsCodeIpcConnection()
            with patch.object(conn,
                              'request',
                              side_effect=ConnectionRefusedError('refused')):
                with self.assertLogs(level='WARNING') as captured:
                    conn.open_file(['/path/to/file.cc'])
                self.assertTrue(
                    any('Could not open files in VS Code window' in line
                        for line in captured.output))


class _ReadableRawIO(io.RawIOBase):
    """Minimal readable RawIOBase for use in _NamedPipeSocket tests."""

    def readable(self):
        return True

    def readinto(self, _b):
        return 0


class NamedPipeSocketTest(unittest.TestCase):
    """Tests for _NamedPipeSocket."""

    def test_sendall_writes_to_pipe(self):
        """sendall() delegates to the underlying pipe's write()."""
        mock_pipe = MagicMock()
        with patch('builtins.open', return_value=mock_pipe):
            sock = _NamedPipeSocket(r'\\.\pipe\test')
            sock.sendall(b'hello')
            mock_pipe.write.assert_called_once_with(b'hello')

    def test_makefile_returns_buffered_reader_wrapping_pipe(self):
        """makefile() wraps the pipe in a BufferedReader."""
        with patch('builtins.open', return_value=_ReadableRawIO()):
            sock = _NamedPipeSocket(r'\\.\pipe\test')
            result = sock.makefile('rb')
            self.assertIsInstance(result, io.BufferedReader)

    def test_close_closes_pipe(self):
        """close() delegates to the underlying pipe's close()."""
        mock_pipe = MagicMock()
        with patch('builtins.open', return_value=mock_pipe):
            sock = _NamedPipeSocket(r'\\.\pipe\test')
            sock.close()
            mock_pipe.close.assert_called_once()

    def test_settimeout_is_noop(self):
        """settimeout() does not raise."""
        mock_pipe = MagicMock()
        with patch('builtins.open', return_value=mock_pipe):
            sock = _NamedPipeSocket(r'\\.\pipe\test')
            sock.settimeout(30)


class WinVsCodeIpcConnectionTest(unittest.TestCase):
    """Tests for _WinVsCodeIpcConnection."""

    PIPE_PATH = r'\\.\pipe\vscode-ipc-abc123'

    def test_init_reads_socket_path_from_env(self):
        """_socket_path is set from VSCODE_GIT_IPC_HANDLE."""
        with patch.dict(os.environ, {'VSCODE_GIT_IPC_HANDLE': self.PIPE_PATH}):
            conn = _WinVsCodeIpcConnection()
            self.assertEqual(conn._socket_path, self.PIPE_PATH)

    def test_init_empty_socket_path_when_env_absent(self):
        """_socket_path is empty when VSCODE_GIT_IPC_HANDLE is not set."""
        with patch.dict(os.environ, {}):
            os.environ.pop('VSCODE_GIT_IPC_HANDLE', None)
            conn = _WinVsCodeIpcConnection()
            self.assertEqual(conn._socket_path, '')

    def test_connect_creates_named_pipe_socket(self):
        """connect() sets sock to a _NamedPipeSocket."""
        with patch.dict(os.environ, {'VSCODE_GIT_IPC_HANDLE': self.PIPE_PATH}):
            conn = _WinVsCodeIpcConnection()
            with patch('builtins.open', return_value=MagicMock()):
                conn.connect()
                self.assertIsInstance(conn.sock, _NamedPipeSocket)

    def test_open_file_skips_when_no_socket_path(self):
        """open_file does nothing when VSCODE_GIT_IPC_HANDLE is not set."""
        with patch.dict(os.environ, {}):
            os.environ.pop('VSCODE_GIT_IPC_HANDLE', None)
            conn = _WinVsCodeIpcConnection()
            with patch.object(conn, 'request') as mock_request:
                conn.open_file(['/some/file.cc'])
                mock_request.assert_not_called()

    def test_open_file_skips_when_files_empty(self):
        """open_file does nothing when the files list is empty."""
        with patch.dict(os.environ, {'VSCODE_GIT_IPC_HANDLE': self.PIPE_PATH}):
            conn = _WinVsCodeIpcConnection()
            with patch.object(conn, 'request') as mock_request:
                conn.open_file([])
                mock_request.assert_not_called()

    def test_open_file_sends_correct_request(self):
        """open_file sends POST / with the correct JSON body and headers."""
        with patch.dict(os.environ, {'VSCODE_GIT_IPC_HANDLE': self.PIPE_PATH}):
            conn = _WinVsCodeIpcConnection()
            mock_response = MagicMock()
            mock_response.status = 200
            mock_response.reason = 'OK'
            mock_response.read.return_value = b''
            with patch.object(conn, 'request') as mock_request, \
                 patch.object(conn, 'getresponse', return_value=mock_response):
                conn.open_file(['/path/to/file.cc'])
                mock_request.assert_called_once()
                method, path, body, headers = mock_request.call_args[0]
                self.assertEqual(method, 'POST')
                self.assertEqual(path, '/')
                payload = json.loads(body)
                self.assertEqual(payload['type'], 'open')
                self.assertEqual(payload['fileURIs'],
                                 ['file:///path/to/file.cc'])
                self.assertTrue(payload['forceReuseWindow'])
                self.assertEqual(headers['Content-Type'], 'application/json')

    def test_open_file_logs_error_on_failure(self):
        """open_file logs a warning and does not raise on connection failure."""
        with patch.dict(os.environ, {'VSCODE_GIT_IPC_HANDLE': self.PIPE_PATH}):
            conn = _WinVsCodeIpcConnection()
            with patch.object(conn,
                              'request',
                              side_effect=ConnectionRefusedError('refused')):
                with self.assertLogs(level='WARNING') as captured:
                    conn.open_file(['/path/to/file.cc'])
                self.assertTrue(
                    any('Could not open files in VS Code window' in line
                        for line in captured.output))


class VsCodeIpcConnectionDispatchTest(unittest.TestCase):
    """Plastform-specific VsCodeIpcConnection test."""

    def test_alias_matches_platform(self):
        if platform.system() == 'Windows':
            self.assertIs(VsCodeIpcConnection, _WinVsCodeIpcConnection)
        else:
            self.assertIs(VsCodeIpcConnection, _PosixVsCodeIpcConnection)


if __name__ == '__main__':
    unittest.main()
