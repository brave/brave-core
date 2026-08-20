#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build Server Protocol server exposing GN Swift targets to sourcekit-lsp.

sourcekit-lsp launches this as a subprocess (configured via `buildServer.json`
at the workspace root) and asks it for the compiler arguments of each Swift
file it opens. The arguments come from the generated ninja files, see
gn_swift_args.py for how they are recovered.

Protocol reference (the sourceKit BSP extensions are experimental and tied to
the toolchain version, developed against Swift 6.3):
https://github.com/swiftlang/sourcekit-lsp/blob/main/Contributor%20Documentation/BSP%20Extensions.md

Log output goes to stderr, which the VS Code Swift extension shows in its
"SourceKit Language Server" output channel. Set BRAVE_BSP_DEBUG=1 for verbose
request logging.
"""

import argparse
import json
import os
import sys
import threading
import traceback
import urllib.parse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import gn_swift_args  # pylint: disable=wrong-import-position

BSP_VERSION = '2.2.0'
SERVER_NAME = 'brave-gn-swift-build-server'
SERVER_VERSION = '0.1.0'

_DEBUG = bool(os.environ.get('BRAVE_BSP_DEBUG'))


def _log(message):
    print(f'[{SERVER_NAME}] {message}', file=sys.stderr, flush=True)


def _debug(message):
    if _DEBUG:
        _log(message)


def _path_to_uri(path):
    return 'file://' + urllib.parse.quote(os.path.abspath(path))


def _uri_to_path(uri):
    if uri.startswith('file://'):
        uri = uri[len('file://'):]
    return os.path.normpath(urllib.parse.unquote(uri))


class TargetIndex:
    """Swift target information, refreshed when build.ninja changes."""

    def __init__(self, source_root, out_dir, workspace_root):
        self.source_root = source_root
        self.out_dir = out_dir
        self.workspace_root = workspace_root
        self._lock = threading.Lock()
        self._build_ninja_mtime = None
        self._targets_by_uri = {}
        self._targets_by_source = {}
        self._arguments_by_uri = {}

    def _in_workspace(self, target):
        """Whether any of `target`'s sources live under the workspace root."""
        prefix = self.workspace_root + os.sep
        return any(
            os.path.realpath(os.path.join(self.out_dir, source)).startswith(
                prefix) for source in target.sources)

    def _build_ninja_path(self):
        return os.path.join(self.out_dir, 'build.ninja')

    def _current_mtime(self):
        try:
            return os.path.getmtime(self._build_ninja_path())
        except OSError:
            return None

    def _refresh_locked(self):
        """Rescans the ninja files if they changed since the last scan."""
        mtime = self._current_mtime()
        if mtime is not None and mtime == self._build_ninja_mtime:
            return
        if mtime is None:
            _log(f'{self._build_ninja_path()} is missing, run `gn gen` first')
            return

        _debug(f'scanning {self.out_dir} for Swift targets')
        targets = gn_swift_args.find_targets(self.out_dir)
        self._targets_by_uri = {target.uri: target for target in targets}
        self._targets_by_source = {}
        for target in targets:
            for source in target.sources:
                path = os.path.realpath(os.path.join(self.out_dir, source))
                self._targets_by_source.setdefault(path, []).append(target)
        self._arguments_by_uri = {}
        self._build_ninja_mtime = mtime
        in_workspace = sum(1 for target in targets
                           if self._in_workspace(target))
        _log(f'found {len(targets)} Swift target(s) in {self.out_dir}, '
             f'{in_workspace} under {self.workspace_root}')

    def targets(self, workspace_only=False):
        """Returns known targets, optionally only those in the workspace."""
        with self._lock:
            self._refresh_locked()
            return [
                target for target in self._targets_by_uri.values()
                if not workspace_only or self._in_workspace(target)
            ]

    def targets_for_source(self, path):
        with self._lock:
            self._refresh_locked()
            return list(self._targets_by_source.get(os.path.realpath(path),
                                                    []))

    def compiler_arguments(self, target_uri):
        """Returns cached swiftc arguments for a target, or None."""
        with self._lock:
            self._refresh_locked()
            if target_uri in self._arguments_by_uri:
                arguments = self._arguments_by_uri[target_uri]
                # Cheap, and the directories can be deleted underneath us at any
                # point during a long-lived editor session.
                gn_swift_args.ensure_cache_dirs(arguments)
                return arguments
            target = self._targets_by_uri.get(target_uri)
            if target is None:
                return None
            arguments = gn_swift_args.compiler_arguments(
                target, self.source_root)
            self._arguments_by_uri[target_uri] = arguments
            return arguments


class BuildServer:
    """JSON-RPC 2.0 server speaking BSP over stdio."""

    def __init__(self, source_root, out_dir, workspace_root):
        self.index = TargetIndex(source_root, out_dir, workspace_root)
        self.source_root = source_root
        self.out_dir = out_dir
        self.workspace_root = workspace_root
        self._shutdown = False

    # -- Transport ----------------------------------------------------------

    def _read_message(self):
        """Reads one LSP-framed JSON-RPC message from stdin."""
        headers = {}
        stream = sys.stdin.buffer
        while True:
            line = stream.readline()
            if not line:
                return None
            line = line.strip()
            if not line:
                break
            name, _, value = line.decode('utf8').partition(':')
            headers[name.strip().lower()] = value.strip()

        length = int(headers.get('content-length', 0))
        if not length:
            return None
        return json.loads(stream.read(length).decode('utf8'))

    def _write_message(self, message):
        payload = json.dumps(message).encode('utf8')
        # Messages are only ever written from the single request-handling loop
        # in run(); the one background thread (index warm-up) never writes.
        sys.stdout.buffer.write(b'Content-Length: %d\r\n\r\n' % len(payload))
        sys.stdout.buffer.write(payload)
        sys.stdout.buffer.flush()

    def _respond(self, request_id, result):
        self._write_message({
            'jsonrpc': '2.0',
            'id': request_id,
            'result': result
        })

    def _respond_error(self, request_id, code, message):
        self._write_message({
            'jsonrpc': '2.0',
            'id': request_id,
            'error': {
                'code': code,
                'message': message
            }
        })

    def run(self):
        _log(f'started, source root {self.source_root}, '
             f'output directory {self.out_dir}')
        while not self._shutdown:
            try:
                message = self._read_message()
            except Exception:  # pylint: disable=broad-except
                _log(f'failed to read message:\n{traceback.format_exc()}')
                break
            if message is None:
                break

            method = message.get('method')
            request_id = message.get('id')
            params = message.get('params') or {}
            _debug(f'--> {method} {json.dumps(params)[:400]}')

            handler = self._handlers().get(method)
            if handler is None:
                # Unknown notifications are ignored; unknown requests get a
                # "method not found" so sourcekit-lsp can probe capabilities.
                if request_id is not None:
                    self._respond_error(request_id, -32601,
                                        f'unhandled method: {method}')
                else:
                    _debug(f'ignoring notification {method}')
                continue

            try:
                result = handler(params)
            except Exception:  # pylint: disable=broad-except
                _log(f'{method} failed:\n{traceback.format_exc()}')
                if request_id is not None:
                    self._respond_error(request_id, -32603,
                                        f'{method} failed, see server log')
                continue

            if request_id is not None:
                self._respond(request_id, result)
                _debug(f'<-- {method} {json.dumps(result)[:400]}')

    def _handlers(self):
        return {
            'build/initialize': self.on_build_initialize,
            'build/initialized': self.on_build_initialized,
            'build/shutdown': self.on_build_shutdown,
            'build/exit': self.on_build_exit,
            'workspace/buildTargets': self.on_workspace_build_targets,
            'buildTarget/sources': self.on_build_target_sources,
            'textDocument/sourceKitOptions': self.
            on_text_document_sourcekit_options,
            'workspace/waitForBuildSystemUpdates': self.
            on_workspace_wait_for_build_system_updates,
        }

    # -- Lifecycle ----------------------------------------------------------

    def on_build_initialize(self, params):
        del params
        index_root = gn_swift_args.cache_root(self.out_dir)
        return {
            'displayName': SERVER_NAME,
            'version': SERVER_VERSION,
            'bspVersion': BSP_VERSION,
            'rootUri': _path_to_uri(self.source_root),
            'capabilities': {
                'languageIds': ['swift']
            },
            'dataKind': 'sourceKit',
            'data': {
                'sourceKitOptionsProvider': True,
                'indexDatabasePath': os.path.join(index_root, 'IndexDatabase'),
            },
        }

    def on_build_initialized(self, params):
        del params
        # Warm the target index so the first opened file responds quickly.
        threading.Thread(target=self.index.targets, daemon=True).start()

    def on_build_shutdown(self, params):
        del params

    def on_build_exit(self, params):
        del params
        self._shutdown = True

    # -- Targets and sources ------------------------------------------------

    def on_workspace_build_targets(self, params):
        del params
        targets = []
        # Only workspace targets are advertised. sourcekit-lsp background
        # indexes everything it is told about, and indexing the ~30 upstream
        # Swift targets in an iOS build means typechecking code the user did not
        # open and filling their module caches. Files in those targets still get
        # full settings when opened, see on_text_document_sourcekit_options.
        for target in self.index.targets(workspace_only=True):
            targets.append({
                'id': {
                    'uri': target.uri
                },
                'displayName': target.label,
                'baseDirectory': _path_to_uri(
                    os.path.join(self.source_root,
                                 target.label[2:].split(':')[0])),
                'tags': ['library'],
                'languageIds': ['swift'],
                'dependencies': [],
                'capabilities': {},
            })
        return {'targets': targets}

    def on_build_target_sources(self, params):
        items = []
        targets_by_uri = {
            target.uri: target
            for target in self.index.targets()
        }
        for identifier in params.get('targets', []):
            uri = identifier.get('uri')
            target = targets_by_uri.get(uri)
            if target is None:
                continue
            sources = []
            for source in target.sources:
                path = os.path.realpath(os.path.join(self.out_dir, source))
                sources.append({
                    'uri': _path_to_uri(path),
                    'kind': 1,
                    'generated': not path.startswith(self.source_root + os.sep)
                    or '/gen/' in path,
                    'dataKind': 'sourceKit',
                    'data': {
                        'language': 'swift'
                    },
                })
            items.append({'target': {'uri': uri}, 'sources': sources})
        return {'items': items}

    def on_workspace_wait_for_build_system_updates(self, params):
        del params
        self.index.targets()

    # -- Compiler arguments -------------------------------------------------

    def on_text_document_sourcekit_options(self, params):
        path = _uri_to_path(params['textDocument']['uri'])
        target_uri = (params.get('target') or {}).get('uri')

        if not target_uri:
            targets = self.index.targets_for_source(path)
            if not targets:
                _log(f'no Swift target owns {path}')
                return None
            target_uri = targets[0].uri

        arguments = self.index.compiler_arguments(target_uri)
        if arguments is None:
            _log(f'no compiler arguments for target {target_uri}')
            return None

        # sourcekit-lsp requires the requested file to be in the argument list.
        # The argument list holds each source as swiftc.py named it, which may
        # differ from the URI the editor opened (e.g. a symlinked checkout), so
        # matching is done on the resolved path. A file added to BUILD.gn but
        # not yet `gn gen`-ed is absent entirely and is appended as opened, so
        # it still typechecks with its module's arguments.
        resolved = os.path.realpath(path)
        if not any(
                argument.endswith('.swift')
                and os.path.realpath(argument) == resolved
                for argument in arguments):
            arguments = arguments + [path]

        return {
            'compilerArguments': arguments,
            'workingDirectory': self.out_dir,
        }


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',
                        default=os.path.join(os.path.dirname(__file__), '..',
                                             '..', '..', '..'),
                        help='path to the Chromium src directory')
    parser.add_argument('--out-dir',
                        default='out/ios_current_link',
                        help='GN output directory, relative to --source-root')
    parser.add_argument(
        '--workspace-root',
        default=None,
        help='only advertise targets with sources under this '
        'directory (default: the `brave` directory inside '
        '--source-root). Pass --source-root to advertise every '
        'Swift target in the build.')
    args = parser.parse_args(argv)

    source_root = os.path.realpath(args.source_root)
    out_dir = os.path.realpath(os.path.join(source_root, args.out_dir))
    workspace_root = os.path.realpath(
        args.workspace_root) if args.workspace_root else os.path.join(
            source_root, 'brave')
    BuildServer(source_root, out_dir, workspace_root).run()
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
