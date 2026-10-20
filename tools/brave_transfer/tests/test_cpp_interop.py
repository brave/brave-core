# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Runs the reference peer against the C++ brave_transfer_peer.

The C++ peer is built from //brave/components/brave_transfer/core:
  autoninja -C out/Component brave_transfer_peer
Point BRAVE_TRANSFER_CPP_PEER at the binary. Skipped when it is not set,
unless BRAVE_TRANSFER_REQUIRE_CPP=1.

Every test runs over pipes and over TCP on 127.0.0.1, with the C++ peer both
listening and dialing.
"""

import asyncio
import json
import os
from pathlib import Path

import pytest

from brave_transfer_ref import proto
from brave_transfer_ref.channel import ProtocolError
from brave_transfer_ref.keys import ChainKeys
from brave_transfer_ref.peer import (ACCEPT_ALL, ACCEPT_FIRST, DECLINE,
                                     INITIATOR, RESPONDER, Peer, PeerConfig,
                                     item_from_path)
from brave_transfer_ref.session import OutgoingItem

TRANSPORTS = ['stdio', 'cpp-listens', 'cpp-dials']
TIMEOUT = 60


@pytest.fixture(scope='module')
def cpp_peer():
    path = os.environ.get('BRAVE_TRANSFER_CPP_PEER')
    if not path or not Path(path).is_file():
        if os.environ.get('BRAVE_TRANSFER_REQUIRE_CPP') == '1':
            pytest.fail('BRAVE_TRANSFER_CPP_PEER is not a file and '
                        'BRAVE_TRANSFER_REQUIRE_CPP=1')
        pytest.skip('BRAVE_TRANSFER_CPP_PEER not set')
    return Path(path)


def _info(name, guid=''):
    return proto.PeerInfo(protocol_version=1,
                          device_name=name,
                          device_type=proto.DEVICE_TYPE_DESKTOP,
                          sync_device_guid=guid,
                          capabilities=3)


async def _close(writer):
    if not writer.is_closing():
        writer.close()
    try:
        await writer.wait_closed()
    except (ConnectionError, OSError):
        pass


async def _run_stdio(cpp_peer, config, cpp_args):
    proc = await asyncio.create_subprocess_exec(
        str(cpp_peer),
        *cpp_args,
        stdin=asyncio.subprocess.PIPE,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE)
    try:
        report = await asyncio.wait_for(
            Peer(config).run(proc.stdout, proc.stdin), TIMEOUT)
    finally:
        if not proc.stdin.is_closing():
            proc.stdin.close()
        stderr = await proc.stderr.read()
        code = await proc.wait()
    return report, code, stderr.decode()


async def _run_cpp_listens(cpp_peer, config, cpp_args):
    proc = await asyncio.create_subprocess_exec(
        str(cpp_peer),
        *cpp_args,
        '--listen',
        '0',
        stdin=asyncio.subprocess.DEVNULL,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE)
    writer = None
    try:
        line = (await asyncio.wait_for(proc.stdout.readline(),
                                       TIMEOUT)).decode()
        assert line.startswith('listening '), line
        reader, writer = await asyncio.open_connection(
            '127.0.0.1', int(line.split()[1]))
        report = await asyncio.wait_for(
            Peer(config).run(reader, writer), TIMEOUT)
    finally:
        if writer:
            await _close(writer)
        stderr = await proc.stderr.read()
        code = await proc.wait()
    return report, code, stderr.decode()


async def _run_cpp_dials(cpp_peer, config, cpp_args):
    result = asyncio.get_running_loop().create_future()

    async def on_connection(reader, writer):
        try:
            result.set_result(await Peer(config).run(reader, writer))
        except Exception as e:  # pylint: disable=broad-except
            result.set_exception(e)
        finally:
            await _close(writer)

    server = await asyncio.start_server(on_connection, '127.0.0.1', 0)
    port = server.sockets[0].getsockname()[1]
    proc = await asyncio.create_subprocess_exec(
        str(cpp_peer),
        *cpp_args,
        '--connect',
        f'127.0.0.1:{port}',
        stdin=asyncio.subprocess.DEVNULL,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.PIPE)
    try:
        report = await asyncio.wait_for(result, TIMEOUT)
    finally:
        server.close()
        stderr = await proc.stderr.read()
        code = await proc.wait()
    return report, code, stderr.decode()


def _run(cpp_peer, transport, config, cpp_args):
    runner = {
        'stdio': _run_stdio,
        'cpp-listens': _run_cpp_listens,
        'cpp-dials': _run_cpp_dials,
    }[transport]
    return asyncio.run(runner(cpp_peer, config, cpp_args))


@pytest.mark.parametrize('transport', TRANSPORTS)
@pytest.mark.parametrize('python_role', [INITIATOR, RESPONDER])
def test_both_directions(cpp_peer, tmp_path, transport, python_role):
    seed = os.urandom(32)
    keys = ChainKeys.from_seed(seed)
    py_file = tmp_path / 'from python.bin'
    py_file.write_bytes(os.urandom(200_003))
    cpp_file = tmp_path / 'from_cpp.dat'
    cpp_file.write_bytes(os.urandom(123_457))
    py_dir, cpp_dir = tmp_path / 'py', tmp_path / 'cpp'
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
        allowed_guids={'cpp-guid'})
    cpp_role = RESPONDER if python_role == INITIATOR else INITIATOR
    report, code, stderr = _run(cpp_peer, transport, config, [
        '--role', cpp_role, '--seed',
        seed.hex(), '--name', 'C++', '--guid', 'cpp-guid', '--allow-guid',
        'python-guid', '--send-file',
        str(cpp_file), '--send-text', 'Hello from C++ 👋', '--expect-offers',
        '1', '--receive-dir',
        str(cpp_dir), '--report',
        str(tmp_path / 'cpp.json')
    ])
    assert code == 0, stderr
    cpp = json.loads((tmp_path / 'cpp.json').read_text())

    assert report.handshake_hash == cpp['handshake_hash']
    assert report.remote_info['device_name'] == 'C++'
    assert cpp['remote_info']['device_name'] == 'Python'
    assert cpp['remote_info']['sync_device_guid'] == 'python-guid'
    assert all(f['status'] == proto.DONE_STATUS_OK
               for f in report.finished + cpp['finished'])
    assert len(report.finished) == 2 and len(cpp['finished']) == 2

    assert (cpp_dir / 'from python.bin').read_bytes() == py_file.read_bytes()
    assert (cpp_dir / 'note.txt').read_text() == long_text
    assert (cpp_dir / 'brave.com').read_text() == 'https://brave.com/transfer'
    assert (py_dir / 'from_cpp.dat').read_bytes() == cpp_file.read_bytes()
    texts = [r.inline_data for r in report.received if r.inline_data]
    assert texts == ['Hello from C++ 👋']
    assert not list(cpp_dir.glob('*.part'))


@pytest.mark.parametrize('transport', TRANSPORTS)
def test_large_file_from_python(cpp_peer, tmp_path, transport):
    seed = os.urandom(32)
    big = tmp_path / 'big.bin'
    big.write_bytes(os.urandom(5 * 1024 * 1024 + 17))
    config = PeerConfig(INITIATOR,
                        ChainKeys.from_seed(seed),
                        _info('Python'),
                        items=[item_from_path(big)])
    report, code, stderr = _run(cpp_peer, transport, config, [
        '--role', RESPONDER, '--seed',
        seed.hex(), '--expect-offers', '1', '--receive-dir',
        str(tmp_path / 'out')
    ])
    assert code == 0, stderr
    assert report.finished[0]['status'] == proto.DONE_STATUS_OK
    assert (tmp_path / 'out' / 'big.bin').read_bytes() == big.read_bytes()


@pytest.mark.parametrize('transport', TRANSPORTS)
def test_large_file_from_cpp(cpp_peer, tmp_path, transport):
    seed = os.urandom(32)
    big = tmp_path / 'big.bin'
    big.write_bytes(os.urandom(3 * 1024 * 1024 + 5))
    config = PeerConfig(RESPONDER,
                        ChainKeys.from_seed(seed),
                        _info('Python'),
                        expect_offers=1,
                        receive_dir=tmp_path / 'out')
    (tmp_path / 'out').mkdir()
    report, code, stderr = _run(
        cpp_peer, transport, config,
        ['--role', INITIATOR, '--seed',
         seed.hex(), '--send-file',
         str(big)])
    assert code == 0, stderr
    assert report.finished[0]['status'] == proto.DONE_STATUS_OK
    assert (tmp_path / 'out' / 'big.bin').read_bytes() == big.read_bytes()


@pytest.mark.parametrize('accept,decision', [(ACCEPT_FIRST, None),
                                             (DECLINE,
                                              proto.DECISION_DECLINE)])
def test_cpp_partial_accept_and_decline(cpp_peer, tmp_path, accept,
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
    _, code, stderr = _run(cpp_peer, 'stdio', config, [
        '--role', RESPONDER, '--seed',
        seed.hex(), '--accept', accept, '--expect-offers', '1',
        '--receive-dir',
        str(tmp_path / 'out'), '--report',
        str(tmp_path / 'cpp.json')
    ])
    assert code == 0, stderr
    cpp = json.loads((tmp_path / 'cpp.json').read_text())
    assert cpp['finished'][0]['decision'] == decision
    names = sorted(r['name'] for r in cpp['received'])
    assert names == ([] if accept == DECLINE else ['first.txt'])


def test_python_declines_cpp(cpp_peer, tmp_path):
    seed = os.urandom(32)
    config = PeerConfig(RESPONDER,
                        ChainKeys.from_seed(seed),
                        _info('Python'),
                        accept=DECLINE,
                        expect_offers=1)
    report, code, stderr = _run(cpp_peer, 'stdio', config, [
        '--role', INITIATOR, '--seed',
        seed.hex(), '--send-text', 'no thanks', '--report',
        str(tmp_path / 'cpp.json')
    ])
    assert code == 0, stderr
    assert report.received == []
    cpp = json.loads((tmp_path / 'cpp.json').read_text())
    assert cpp['finished'][0]['decision'] == proto.DECISION_DECLINE


@pytest.mark.parametrize('transport', TRANSPORTS)
@pytest.mark.parametrize('python_role', [INITIATOR, RESPONDER])
def test_other_sync_chain_fails_on_both_sides(cpp_peer, transport,
                                              python_role):
    config = PeerConfig(python_role, ChainKeys.from_seed(os.urandom(32)),
                        _info('Python'))
    cpp_role = RESPONDER if python_role == INITIATOR else INITIATOR
    with pytest.raises((ProtocolError, ConnectionError, EOFError,
                        asyncio.IncompleteReadError)):
        _run(cpp_peer, transport, config,
             ['--role', cpp_role, '--seed',
              os.urandom(32).hex()])


@pytest.mark.parametrize('python_role', [INITIATOR, RESPONDER])
def test_disallowed_guid_is_rejected(cpp_peer, python_role):
    seed = os.urandom(32)
    config = PeerConfig(python_role, ChainKeys.from_seed(seed),
                        _info('Python', 'python-guid'))
    cpp_role = RESPONDER if python_role == INITIATOR else INITIATOR
    try:
        _, code, stderr = _run(cpp_peer, 'stdio', config, [
            '--role', cpp_role, '--seed',
            seed.hex(), '--allow-guid', 'someone-else'
        ])
    except (ProtocolError, ConnectionError, EOFError,
            asyncio.IncompleteReadError):
        return
    assert code != 0
    assert 'not in the sync chain' in stderr
