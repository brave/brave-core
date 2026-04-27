# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import http.client
import io
import json
import logging
import os
from pathlib import Path
import platform
import socket


class _VsCodeIpcConnectionBase(http.client.HTTPConnection):
    """Base for platform-specific VS Code IPC HTTP connections."""

    def __init__(self, env_var: str):
        super().__init__('vscode')
        self._socket_path = os.environ.get(env_var, '')

    def open_file(self, files: list) -> None:
        """Opens files in the VS Code window that owns this terminal.

        Communicates directly with the IPC socket rather than spawning
        `code`, so the request is routed to the specific window whose
        extension host created the socket — not whichever window happens
        to be in the foreground.
        """
        if not self._socket_path or not files:
            return
        file_uris = [Path(str(f)).resolve().as_uri() for f in files]
        body = json.dumps({
            'type': 'open',
            'fileURIs': file_uris,
            'forceReuseWindow': True,
        }).encode()
        logging.debug('VS Code IPC request: socket=%s,  body=%s',
                      self._socket_path, body.decode())
        try:
            self.request('POST', '/', body,
                         {'Content-Type': 'application/json'})
            resp = self.getresponse()
            logging.debug('VS Code IPC response: %d %s\n', resp.status,
                          resp.reason)
        except Exception as e:
            logging.warning('Could not open files in VS Code window: %s', e)


class _PosixVsCodeIpcConnection(_VsCodeIpcConnectionBase):
    """VS Code IPC connection via Unix domain socket (POSIX)."""

    def __init__(self):
        """Reads the IPC socket path from VSCODE_IPC_HOOK_CLI.

        VS Code sets this variable in every integrated terminal it spawns,
        pointing to the Unix socket owned by that window's extension host.
        """
        super().__init__('VSCODE_IPC_HOOK_CLI')

    def connect(self) -> None:
        """Overrides HTTPConnection.connect to use a Unix domain socket."""
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self._socket_path)


class _NamedPipeSocket:
    """Socket-like wrapper around a Windows named pipe for HTTPConnection."""

    def __init__(self, path: str):
        """Opens the named pipe at path for binary read/write."""
        self._pipe = open(path, 'r+b', buffering=0)

    def sendall(self, data: bytes) -> None:
        """Writes data to the pipe (called by HTTPConnection)."""
        self._pipe.write(data)

    def makefile(self, _mode: str, _bufsize: int = -1) -> io.BufferedReader:
        """Returns a BufferedReader over the pipe for HTTPResponse to use."""
        return io.BufferedReader(self._pipe)

    def close(self) -> None:
        """Closes the underlying pipe handle."""
        self._pipe.close()

    def settimeout(self, _timeout) -> None:
        """No-op: named pipes do not support socket-style timeouts."""


class _WinVsCodeIpcConnection(_VsCodeIpcConnectionBase):
    """VS Code IPC connection via Windows named pipe."""

    def __init__(self):
        """Reads the IPC socket path from VSCODE_GIT_IPC_HANDLE.

        VS Code sets this variable in every integrated terminal it spawns,
        pointing to the named pipe owned by that window's extension host.
        """
        super().__init__('VSCODE_GIT_IPC_HANDLE')

    def connect(self) -> None:
        """Overrides HTTPConnection.connect to use a Windows named pipe."""
        self.sock = _NamedPipeSocket(self._socket_path)


VsCodeIpcConnection = (_WinVsCodeIpcConnection if platform.system()
                       == 'Windows' else _PosixVsCodeIpcConnection)
