# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Runs the reference peer against the Swift BraveTransferPeer over pipes.

Skipped when no Swift toolchain is found, unless
BRAVE_TRANSFER_REQUIRE_SWIFT=1. Set SWIFT to pick the swift binary.
"""

import asyncio
import json
import os
import shutil
import subprocess
from pathlib import Path

import pytest

from brave_transfer_ref import proto
from brave_transfer_ref.channel import ProtocolError
from brave_transfer_ref.keys import ChainKeys
from brave_transfer_ref.peer import (ACCEPT_ALL, ACCEPT_FIRST, DECLINE,
                                     INITIATOR, RESPONDER, Peer, PeerConfig,
                                     item_from_path)
from brave_transfer_ref.session import OutgoingItem

PACKAGE_DIR = Path(__file__).resolve().parents[3] / 'components' / \
    'brave_transfer' / 'swift'


@pytest.fixture(scope='module')
def swift_peer():
    swift = os.environ.get('SWIFT') or shutil.which('swift')
    if not swift:
        if os.environ.get('BRAVE_TRANSFER_REQUIRE_SWIFT') == '1':
            pytest.fail('swift not found and BRAVE_TRANSFER_REQUIRE_SWIFT=1')
        pytest.skip('swift not found')
    subprocess.run(
        [swift, 'build', '--product', 'BraveTransferPeer'],
        cwd=PACKAGE_DIR,
        check=True,
        stdout=subprocess.DEVNULL)
    bin_dir = subprocess.run([swift, 'build', '--show-bin-path'],
                             cwd=PACKAGE_DIR,
                             check=True,
                             capture_output=True,
                             text=True).stdout.strip()
    return Path(bin_dir) / 'BraveTransferPeer'


def _info(name, guid=''):
    return proto.PeerInfo(protocol_version=1,
                          device_name=name,
                          device_type=proto.DEVICE_TYPE_DESKTOP,
                          sync_device_guid=guid,
                          capabilities=3)


async def _run(swift_peer, config: PeerConfig, swift_args):
    proc = await asyncio.create_subprocess_exec(
        str(swift_peer),
        *swift_args,
        stdin=asyncio.subprocess.PIPE,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE)
    try:
        report = await asyncio.wait_for(
            Peer(config).run(proc.stdout, proc.stdin), 60)
    finally:
        if proc.stdin.can_write_eof() and not proc.stdin.is_closing():
            proc.stdin.close()
        stderr = await proc.stderr.read()
        code = await proc.wait()
    return report, code, stderr.decode()


def _swift_report(path: Path):
    return json.loads(path.read_text())


@pytest.mark.parametrize('python_role', [INITIATOR, RESPONDER])
def test_both_directions(swift_peer, tmp_path, python_role):
    seed = os.urandom(32)
    keys = ChainKeys.from_seed(seed)
    py_file = tmp_path / 'from python.bin'
    py_file.write_bytes(os.urandom(200_003))
    swift_file = tmp_path / 'from_swift.dat'
    swift_file.write_bytes(os.urandom(123_457))
    py_dir, swift_dir = tmp_path / 'py', tmp_path / 'swift'
    py_dir.mkdir()
    long_text = 'Brave ' * 2000
    config = PeerConfig(
        python_role,
        keys,
        _info('Python', 'python-guid'),
        items=[
            item_from_path(py_file),
            OutgoingItem.from_bytes(proto.ITEM_KIND_TEXT, 'note.txt',
                                    'text/plain', long_text.encode()),
            OutgoingItem.from_bytes(proto.ITEM_KIND_URL, 'brave.com',
                                    'text/uri-list',
                                    b'https://brave.com/transfer'),
        ],
        expect_offers=1,
        receive_dir=py_dir,
        allowed_guids={'swift-guid'})
    swift_role = RESPONDER if python_role == INITIATOR else INITIATOR
    report, code, stderr = asyncio.run(
        _run(swift_peer, config, [
            '--role', swift_role, '--seed', seed.hex(), '--name', 'Swift',
            '--guid', 'swift-guid', '--allow-guid', 'python-guid',
            '--send-file',
            str(swift_file), '--send-text', 'Hello from Swift 👋',
            '--expect-offers', '1', '--receive-dir',
            str(swift_dir), '--report',
            str(tmp_path / 'swift.json')
        ]))
    assert code == 0, stderr
    swift = _swift_report(tmp_path / 'swift.json')

    assert report.handshake_hash == swift['handshake_hash']
    assert report.remote_info['device_name'] == 'Swift'
    assert swift['remote_info']['device_name'] == 'Python'
    assert all(f['status'] == proto.DONE_STATUS_OK
               for f in report.finished + swift['finished'])

    assert (swift_dir / 'from python.bin').read_bytes() == py_file.read_bytes()
    assert (swift_dir / 'note.txt').read_text() == long_text
    assert (swift_dir / 'brave.com').read_text() == 'https://brave.com/transfer'
    assert (py_dir / 'from_swift.dat').read_bytes() == swift_file.read_bytes()
    texts = [r.inline_data for r in report.received if r.inline_data]
    assert texts == ['Hello from Swift 👋']


def test_large_file_from_python(swift_peer, tmp_path):
    seed = os.urandom(32)
    big = tmp_path / 'big.bin'
    big.write_bytes(os.urandom(5 * 1024 * 1024 + 17))
    config = PeerConfig(INITIATOR,
                        ChainKeys.from_seed(seed),
                        _info('Python'),
                        items=[item_from_path(big)])
    report, code, stderr = asyncio.run(
        _run(swift_peer, config, [
            '--role', RESPONDER, '--seed',
            seed.hex(), '--expect-offers', '1', '--receive-dir',
            str(tmp_path / 'out')
        ]))
    assert code == 0, stderr
    assert report.finished[0]['status'] == proto.DONE_STATUS_OK
    assert (tmp_path / 'out' / 'big.bin').read_bytes() == big.read_bytes()


@pytest.mark.parametrize('accept,decision', [(ACCEPT_FIRST, None),
                                             (DECLINE,
                                              proto.DECISION_DECLINE)])
def test_swift_partial_accept_and_decline(swift_peer, tmp_path, accept,
                                          decision):
    seed = os.urandom(32)
    f = tmp_path / 'skipped.bin'
    f.write_bytes(os.urandom(70_000))
    config = PeerConfig(INITIATOR,
                        ChainKeys.from_seed(seed),
                        _info('Python'),
                        items=[
                            OutgoingItem.from_bytes(proto.ITEM_KIND_TEXT,
                                                    'first.txt', 'text/plain',
                                                    b'first'),
                            item_from_path(f)
                        ])
    _, code, stderr = asyncio.run(
        _run(swift_peer, config, [
            '--role', RESPONDER, '--seed',
            seed.hex(), '--accept', accept, '--expect-offers', '1',
            '--receive-dir',
            str(tmp_path / 'out'), '--report',
            str(tmp_path / 'swift.json')
        ]))
    assert code == 0, stderr
    swift = _swift_report(tmp_path / 'swift.json')
    assert swift['finished'][0]['decision'] == decision
    names = sorted(r['name'] for r in swift['received'])
    assert names == ([] if accept == DECLINE else ['first.txt'])


def test_python_declines_swift(swift_peer, tmp_path):
    seed = os.urandom(32)
    config = PeerConfig(RESPONDER,
                        ChainKeys.from_seed(seed),
                        _info('Python'),
                        accept=DECLINE,
                        expect_offers=1)
    report, code, stderr = asyncio.run(
        _run(swift_peer, config, [
            '--role', INITIATOR, '--seed',
            seed.hex(), '--send-text', 'no thanks', '--report',
            str(tmp_path / 'swift.json')
        ]))
    assert code == 0, stderr
    assert report.received == []
    swift = _swift_report(tmp_path / 'swift.json')
    assert swift['finished'][0]['decision'] == proto.DECISION_DECLINE


@pytest.mark.parametrize('python_role', [INITIATOR, RESPONDER])
def test_other_sync_chain_fails_on_both_sides(swift_peer, python_role):
    config = PeerConfig(python_role, ChainKeys.from_seed(os.urandom(32)),
                        _info('Python'))
    swift_role = RESPONDER if python_role == INITIATOR else INITIATOR
    with pytest.raises((ProtocolError, ConnectionError)):
        asyncio.run(
            _run(swift_peer, config,
                 ['--role', swift_role, '--seed',
                  os.urandom(32).hex()]))
