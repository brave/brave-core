# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import asyncio
import os

import pytest

from brave_transfer_ref import proto
from brave_transfer_ref.channel import ProtocolError
from brave_transfer_ref.keys import ChainKeys
from brave_transfer_ref.peer import (DECLINE, INITIATOR, RESPONDER, Peer,
                                     PeerConfig, item_from_path)
from brave_transfer_ref.session import OutgoingItem


def _info(name, guid=''):
    return proto.PeerInfo(protocol_version=1,
                          device_name=name,
                          sync_device_guid=guid)


async def _pair(initiator: PeerConfig, responder: PeerConfig):
    done = asyncio.get_running_loop().create_future()

    async def on_client(reader, writer):
        try:
            done.set_result(await Peer(responder).run(reader, writer))
        except Exception as e:  # pylint: disable=broad-except
            done.set_exception(e)
        finally:
            writer.close()

    server = await asyncio.start_server(on_client, '127.0.0.1', 0)
    port = server.sockets[0].getsockname()[1]
    try:
        reader, writer = await asyncio.open_connection('127.0.0.1', port)
        try:
            i_report = await asyncio.wait_for(
                Peer(initiator).run(reader, writer), 20)
        finally:
            writer.close()
        r_report = await asyncio.wait_for(done, 20)
    finally:
        server.close()
    return i_report, r_report


def test_files_and_text_both_ways(tmp_path):
    keys = ChainKeys.from_seed(os.urandom(32))
    big = tmp_path / 'big.bin'
    big.write_bytes(os.urandom(300_000))
    i_dir, r_dir = tmp_path / 'i', tmp_path / 'r'
    i_dir.mkdir()
    r_dir.mkdir()
    initiator = PeerConfig(INITIATOR, keys, _info('Mac', 'mac-guid'),
                           items=[item_from_path(big)],
                           expect_offers=1,
                           receive_dir=i_dir,
                           allowed_guids={'phone-guid'})
    responder = PeerConfig(RESPONDER, keys, _info('iPhone', 'phone-guid'),
                           items=[
                               OutgoingItem.from_bytes(
                                   proto.ITEM_KIND_URL, 'u', 'text/uri-list',
                                   b'https://brave.com')
                           ],
                           expect_offers=1,
                           receive_dir=r_dir)
    i_report, r_report = asyncio.run(_pair(initiator, responder))
    assert (r_dir / 'big.bin').read_bytes() == big.read_bytes()
    assert i_report.received[0].inline_data == 'https://brave.com'
    assert i_report.handshake_hash == r_report.handshake_hash
    assert r_report.remote_info['device_name'] == 'Mac'
    assert all(f['status'] == proto.DONE_STATUS_OK
               for f in i_report.finished + r_report.finished)


def test_decline(tmp_path):
    keys = ChainKeys.from_seed(os.urandom(32))
    item = OutgoingItem.from_bytes(proto.ITEM_KIND_TEXT, 't', 'text/plain',
                                   b'hello')
    i_report, r_report = asyncio.run(
        _pair(PeerConfig(INITIATOR, keys, _info('Mac'), items=[item]),
              PeerConfig(RESPONDER, keys, _info('iPhone'), accept=DECLINE,
                         expect_offers=1)))
    assert i_report.finished[0]['decision'] == proto.DECISION_DECLINE
    assert r_report.received == []


def test_other_sync_chain_is_rejected():
    initiator = PeerConfig(INITIATOR, ChainKeys.from_seed(bytes(32)),
                           _info('Mac'))
    responder = PeerConfig(RESPONDER, ChainKeys.from_seed(b'\1' * 32),
                           _info('iPhone'))
    with pytest.raises((ProtocolError, ConnectionError)):
        asyncio.run(_pair(initiator, responder))


def test_device_outside_allowed_guids_is_rejected():
    keys = ChainKeys.from_seed(os.urandom(32))
    initiator = PeerConfig(INITIATOR, keys, _info('Mac'),
                           allowed_guids={'someone-else'})
    responder = PeerConfig(RESPONDER, keys, _info('iPhone', 'phone'))
    with pytest.raises((ProtocolError, ConnectionError)):
        asyncio.run(_pair(initiator, responder))
