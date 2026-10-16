# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Drives the dev build over the Chrome DevTools Protocol (CDP).

Pure Python, no packages: a small WebSocket client, a launcher for a test
profile, and helpers for tabs and prefs.

A test profile lives where `pnpm start --user_data_dir_name=<name>` puts it,
outside the repo and outside /tmp:

  macOS:  ~/Library/Application Support/BraveSoftware/<name>
  Linux:  ~/.config/BraveSoftware/<name>

The files of the tools (state, reports, browser logs) go in the `ai_chat_cdp`
folder of the profile, so deleting the profile deletes them too.
"""

import base64
import glob
import json
import os
import socket
import struct
import subprocess
import sys
import time
import urllib.request

BRAVE_DIR = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
)
SRC_DIR = os.path.dirname(BRAVE_DIR)
DEFAULT_BUILD_DIR = os.path.join(SRC_DIR, 'out', 'Component_arm64')
PORT = 9223

# Walks the shadow roots and gives the first element that |pred| accepts.
FIND = (
    '(pred)=>{let el=null;const walk=r=>{for(const e of '
    "r.querySelectorAll('*')){if(el)return;if(pred(e)){el=e;return}"
    'if(e.shadowRoot)walk(e.shadowRoot)}};walk(document);return el}'
)


def log(*args):
    print(*args, flush=True)


def profiles_dir():
    """The folder of `pnpm start --user_data_dir_name`."""
    home = os.path.expanduser('~')
    if sys.platform == 'darwin':
        return os.path.join(
            home, 'Library', 'Application Support', 'BraveSoftware'
        )
    return os.path.join(home, '.config', 'BraveSoftware')


def profile_path(name):
    return os.path.join(profiles_dir(), name)


def dev_profile_path():
    """The profile of a plain `pnpm start`, which has the embedder model."""
    return profile_path('Brave-Browser-Development')


def tools_dir(profile):
    path = os.path.join(profile, 'ai_chat_cdp')
    os.makedirs(path, exist_ok=True)
    return path


def browser_binary(build_dir=DEFAULT_BUILD_DIR):
    """The binary that `pnpm start` runs. On macOS the brave_helper file names
    it, with its spaces escaped for a shell."""
    if sys.platform != 'darwin':
        return os.path.join(build_dir, 'brave')
    with open(os.path.join(build_dir, 'brave_helper'), encoding='utf-8') as f:
        return f.read().strip().replace('\\ ', ' ')


def build_time(build_dir=DEFAULT_BUILD_DIR):
    """When the browser was built last: the newest of its library and its
    resource packs. A change of only WebUI files or strings builds the packs
    again, and not the library."""
    library = 'libchrome_dll.dylib' if sys.platform == 'darwin' else 'chrome'
    paths = [os.path.join(build_dir, library)]
    if sys.platform == 'darwin':
        paths += glob.glob(
            os.path.join(
                build_dir,
                '*.app',
                'Contents',
                'Frameworks',
                '*.framework',
                'Versions',
                'Current',
                'Resources',
                '**',
                '*.pak',
            ),
            recursive=True,
        )
    else:
        paths += glob.glob(os.path.join(build_dir, '*.pak'))
        paths += glob.glob(os.path.join(build_dir, 'locales', '*.pak'))
    return max(os.path.getmtime(path) for path in paths if os.path.exists(path))


def check_build(build_dir=DEFAULT_BUILD_DIR):
    """Stops when the code of the browser changed after it was built: a test
    of a binary without that code says nothing. The code is the tree without
    tools/ and Markdown files, which do not go into the browser.

    The files to check are the ones that differ from HEAD at the time of the
    build: later commits, and changes not committed yet. Such a file was in
    the build when it did not change on disk after the build, so a commit
    made after the build of its changes does not count."""
    built = build_time(build_dir)

    def git(*args):
        return subprocess.run(
            ['git', '-C', BRAVE_DIR] + list(args),
            capture_output=True,
            text=True,
            check=False,
        )

    # The newest HEAD that is older than the binary.
    reflog = git('log', '-g', '-200', '--format=%H %gd', '--date=unix')
    built_head = None
    for line in reflog.stdout.splitlines():
        commit, entry = line.split(' ', 1)
        if int(entry.split('{')[1].rstrip('}')) <= built:
            built_head = commit
            break
    # Without a commit argument after it, the diff is to the files on disk.
    changed = (
        git(
            'diff',
            '--name-only',
            built_head,
            '--',
            '.',
            ':(exclude)tools',
            ':(exclude)*.md',
        )
        if built_head
        else None
    )
    newer = None
    if changed is not None and changed.returncode == 0:
        newer = []
        for name in changed.stdout.split():
            path = os.path.join(BRAVE_DIR, name)
            # A deleted file has no time. It counts as changed.
            if not os.path.exists(path) or os.path.getmtime(path) > built:
                newer.append(name)
    if newer is None or newer:
        raise SystemExit(
            f'The browser ({time.ctime(built)}) is older than its code'
            + (
                f' ({len(newer)} files changed, for example {newer[0]})'
                if newer
                else ''
            )
            + '. Build it first.'
        )


def http(method, path):
    request = urllib.request.Request(
        f'http://127.0.0.1:{PORT}{path}', method=method
    )
    with urllib.request.urlopen(request, timeout=10) as response:
        body = response.read()
    try:
        return json.loads(body)
    except ValueError:
        return body.decode(errors='replace')


class WebSocket:
    """A CDP connection to one target."""

    def __init__(self, url, timeout=30):
        host, rest = url[len('ws://') :].split('/', 1)
        name, port = host.split(':')
        self.sock = socket.create_connection((name, int(port)), timeout=timeout)
        key = base64.b64encode(os.urandom(16)).decode()
        self.sock.sendall(
            (
                f'GET /{rest} HTTP/1.1\r\nHost: {host}\r\n'
                'Upgrade: websocket\r\nConnection: Upgrade\r\n'
                f'Sec-WebSocket-Key: {key}\r\n'
                'Sec-WebSocket-Version: 13\r\n\r\n'
            ).encode()
        )
        buf = b''
        while b'\r\n\r\n' not in buf:
            buf += self.sock.recv(4096)
        self.buf = buf.split(b'\r\n\r\n', 1)[1]
        self.next_id = 0

    def _read(self, size):
        while len(self.buf) < size:
            chunk = self.sock.recv(1 << 16)
            if not chunk:
                raise EOFError
            self.buf += chunk
        out, self.buf = self.buf[:size], self.buf[size:]
        return out

    def send(self, message):
        data = json.dumps(message).encode()
        header = bytearray([0x81])
        size = len(data)
        if size < 126:
            header.append(0x80 | size)
        elif size < 65536:
            header.append(0x80 | 126)
            header += struct.pack('>H', size)
        else:
            header.append(0x80 | 127)
            header += struct.pack('>Q', size)
        mask = os.urandom(4)
        self.sock.sendall(
            bytes(header)
            + mask
            + bytes(b ^ mask[i % 4] for i, b in enumerate(data))
        )

    def receive(self):
        payload = b''
        while True:
            first, second = self._read(2)
            size = second & 0x7F
            if size == 126:
                size = struct.unpack('>H', self._read(2))[0]
            elif size == 127:
                size = struct.unpack('>Q', self._read(8))[0]
            payload += self._read(size)
            if first & 0x80:
                return json.loads(payload)

    def call(self, method, **params):
        self.next_id += 1
        self.send({'id': self.next_id, 'method': method, 'params': params})
        while True:
            message = self.receive()
            if message.get('id') == self.next_id:
                if 'error' in message:
                    raise RuntimeError(message['error'])
                return message.get('result')

    def eval(self, expression):
        result = self.call(
            'Runtime.evaluate',
            expression=expression,
            awaitPromise=True,
            returnByValue=True,
        )
        if 'exceptionDetails' in result:
            raise RuntimeError(result['exceptionDetails'])
        return result['result'].get('value')

    def screenshot(self, path):
        self.eval(
            'document.fonts.ready.then(()=>new Promise(r=>'
            'setTimeout(r,1000))).then(()=>1)'
        )
        result = self.call('Page.captureScreenshot', format='png')
        with open(path, 'wb') as f:
            f.write(base64.b64decode(result['data']))


def targets():
    return http('GET', '/json/list')


def connect(target_id):
    for target in targets():
        if target['id'] == target_id:
            return WebSocket(target['webSocketDebuggerUrl'])
    raise RuntimeError(f'no target {target_id}')


class Browser:
    """The dev build on one test profile, with CDP on.

    with Browser(profile, log_path) as browser:
        ws, tab = browser.new_tab('brave://leo-ai/')
    """

    def __init__(
        self, profile, log_path, extra_args=(), build_dir=DEFAULT_BUILD_DIR
    ):
        self.profile = profile
        self.log_path = log_path
        self.extra_args = list(extra_args)
        self.binary = browser_binary(build_dir)
        self.process = None
        self.log_file = None

    def __enter__(self):
        self.launch()
        return self

    def __exit__(self, *exc):
        self.quit()

    def running(self):
        return (
            subprocess.run(
                ['pgrep', '-f', f'user-data-dir={self.profile}'],
                capture_output=True,
                check=False,
            ).returncode
            == 0
        )

    def launch(self):
        if self.running():
            raise RuntimeError(f'a browser already uses {self.profile}')
        # The flags of `pnpm start`, and the CDP ones.
        args = [
            self.binary,
            '--enable-logging',
            '--v=0',
            '--use-mock-keychain',
            '--disable-brave-update',
            '--disable-features=DialMediaRouteProvider',
            f'--remote-debugging-port={PORT}',
            f'--user-data-dir={self.profile}',
            '--no-first-run',
            '--no-default-browser-check',
            '--remote-allow-origins=*',
        ] + self.extra_args
        # The browser outlives this call; quit() closes both.
        # pylint: disable=consider-using-with
        self.log_file = open(self.log_path, 'w', encoding='utf-8')
        self.process = subprocess.Popen(
            args, stdout=self.log_file, stderr=subprocess.STDOUT
        )
        # pylint: enable=consider-using-with
        for _ in range(90):
            try:
                http('GET', '/json/version')
                time.sleep(4)
                self.close_extra_tabs()
                return
            except OSError:
                time.sleep(1)
        raise RuntimeError('CDP never came up')

    def quit(self):
        try:
            ws = WebSocket(http('GET', '/json/version')['webSocketDebuggerUrl'])
            ws.send({'id': 1, 'method': 'Browser.close', 'params': {}})
        except OSError:
            pass
        try:
            self.process.wait(timeout=60)
        except subprocess.TimeoutExpired:
            self.process.kill()
        self.log_file.close()

    @staticmethod
    def close_extra_tabs():
        """Closes the tabs that a restored session brings back, but one.
        Leo's pages share a renderer, so a crowd of them is slow."""
        pages = [t for t in targets() if t['type'] == 'page']
        for tab in pages[1:]:
            Browser.close_tab(tab['id'])

    @staticmethod
    def new_tab(url, settle=4):
        before = {t['id'] for t in targets()}
        http('PUT', f'/json/new?{url}')
        for _ in range(30):
            new = [
                t
                for t in targets()
                if t['id'] not in before and t['type'] == 'page'
            ]
            if new:
                break
            time.sleep(0.5)
        else:
            raise RuntimeError(f'no tab for {url}')
        time.sleep(settle)
        ws = connect(new[0]['id'])
        ws.call('Page.enable')
        return ws, new[0]['id']

    @staticmethod
    def close_tab(target_id):
        try:
            http('PUT', f'/json/close/{target_id}')
        except OSError:
            pass

    def set_pref(self, key, value):
        """Sets a pref through settingsPrivate, the way the settings page
        does, so the pref is not seen as changed behind the browser."""
        ws, tab = self.new_tab('brave://settings/privacy')
        done = ws.eval(
            'new Promise(r=>chrome.settingsPrivate.setPref('
            f'{json.dumps(key)}, {json.dumps(value)}, "", ok=>r(ok)))'
        )
        self.close_tab(tab)
        return done

    def get_prefs(self, pattern):
        ws, tab = self.new_tab('brave://settings/privacy')
        prefs = ws.eval(
            'new Promise(r=>chrome.settingsPrivate.getAllPrefs('
            f'p=>r(p.filter(x=>new RegExp({json.dumps(pattern)})'
            '.test(x.key)).map(x=>[x.key,x.value]))))'
        )
        self.close_tab(tab)
        return prefs


def find_in(element_tag, body, visible=True):
    """JS that runs |body| with `sec` bound to the first element with the tag
    |element_tag| (through shadow roots). Gives null when there is none."""
    condition = f"e.tagName==={json.dumps(element_tag)}"
    if visible:
        condition += '&&e.getClientRects().length>0'
    return (
        f'(async()=>{{const f={FIND};const sec=f(e=>{condition});'
        f'if(!sec)return null;{body}}})()'
    )
