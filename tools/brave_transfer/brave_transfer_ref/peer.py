# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""asyncio runner that plays one device over any byte stream.

The stream can be a TCP connection, a discovered mDNS service, or the stdio
pipes of another implementation (for example the Swift BraveTransferPeer).

A run ends when this side has finished its own work (its offer completed and
`expect_offers` incoming transfers finished). It then half-closes its write
side and reads until the remote does the same, so both sides can finish
cleanly without an extra "goodbye" message.
"""

import asyncio
import hashlib
import os
import struct
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, List, Optional

from . import proto
from .channel import (InitiatorHandshake, ProtocolError, ResponderHandshake,
                      length_prefix)
from .keys import ChainKeys, epoch_for, fingerprint, x25519_public
from .sanitize import sanitize_filename
from .session import (ChunkReceived, InlineItemReceived, ItemReceived, Limits,
                      OfferReceived, OutgoingItem, ResponseReceived,
                      TransferFinished, TransferSession)

INITIATOR = 'initiator'
RESPONDER = 'responder'

ACCEPT_ALL = 'all'
ACCEPT_FIRST = 'first'
DECLINE = 'decline'

_POLL_INTERVAL = 1.0


@dataclass
class PeerConfig:
    role: str
    keys: ChainKeys
    info: proto.PeerInfo
    static_private: bytes = field(default_factory=lambda: os.urandom(32))
    items: List[OutgoingItem] = field(default_factory=list)
    accept: str = ACCEPT_ALL
    expect_offers: int = 0
    receive_dir: Optional[Path] = None
    allowed_guids: Optional[set] = None
    limits: Limits = field(default_factory=Limits)
    clock: Callable[[], float] = time.time


@dataclass
class ReceivedItem:
    transfer_id: str
    item_id: int
    kind: int
    name: str
    size: int
    sha256: str
    saved_as: Optional[str] = None
    inline_data: Optional[str] = None


@dataclass
class Report:
    remote_info: dict = None
    remote_fingerprint: str = ''
    handshake_hash: str = ''
    received: List[ReceivedItem] = field(default_factory=list)
    finished: List[dict] = field(default_factory=list)

    def to_json(self):
        return {
            'remote_info': self.remote_info,
            'remote_fingerprint': self.remote_fingerprint,
            'handshake_hash': self.handshake_hash,
            'received': [vars(r) for r in self.received],
            'finished': self.finished,
        }


def item_from_path(path: Path, kind=proto.ITEM_KIND_FILE,
                   mime_type='application/octet-stream') -> OutgoingItem:
    path = Path(path)
    size = path.stat().st_size
    digest = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            digest.update(block)

    def read(offset, length):
        with path.open('rb') as f:
            f.seek(offset)
            return f.read(length)

    return OutgoingItem(kind, path.name, mime_type, size, digest.digest(),
                        b'', read)


def _unique_path(directory: Path, name: str) -> Path:
    candidate = directory / name
    stem, dot, ext = name.partition('.')
    n = 1
    while candidate.exists() or candidate.with_name(candidate.name +
                                                    '.part').exists():
        candidate = directory / f'{stem} ({n}){dot}{ext}'
        n += 1
    return candidate


async def _read_message(reader: asyncio.StreamReader) -> Optional[bytes]:
    """Returns None on a clean EOF between messages."""
    try:
        header = await reader.readexactly(2)
    except asyncio.IncompleteReadError as e:
        if not e.partial:
            return None
        raise ProtocolError('connection closed mid-message') from e
    try:
        return await reader.readexactly(struct.unpack('>H', header)[0])
    except asyncio.IncompleteReadError as e:
        raise ProtocolError('connection closed mid-message') from e


async def _read_handshake(reader) -> bytes:
    message = await _read_message(reader)
    if message is None:
        raise ProtocolError('connection closed during handshake')
    return message


class Peer:

    def __init__(self, config: PeerConfig):
        self.config = config
        self.report = Report()
        self._open_files = {}
        self._outgoing_done = not config.items
        self._incoming_finished = 0

    async def _handshake(self, reader, writer):
        c = self.config
        now = c.clock()
        if c.role == INITIATOR:
            hs = InitiatorHandshake(c.static_private, c.keys.psk,
                                    c.keys.tag(epoch_for(now)), c.info)
            writer.write(length_prefix(hs.message1()))
            await writer.drain()
            hs.read_message2(await _read_handshake(reader))
            writer.write(length_prefix(hs.message3()))
            await writer.drain()
        else:
            tags = c.keys.accepted_tags(now)
            hs = ResponderHandshake(
                c.static_private, lambda tag: c.keys.psk
                if tag in tags else None, c.info)
            hs.read_message1(await _read_handshake(reader))
            writer.write(length_prefix(hs.message2()))
            await writer.drain()
            hs.read_message3(await _read_handshake(reader))
        channel = hs.channel()
        if c.allowed_guids is not None and \
                channel.remote_info.sync_device_guid not in c.allowed_guids:
            raise ProtocolError('remote device is not in the sync chain')
        self.report.remote_info = channel.remote_info.to_json()
        self.report.remote_fingerprint = fingerprint(channel.remote_static)
        self.report.handshake_hash = channel.handshake_hash.hex()
        return channel

    @property
    def _work_done(self):
        return self._outgoing_done and \
            self._incoming_finished >= self.config.expect_offers

    def _decide(self, offer):
        if self.config.accept == ACCEPT_ALL:
            return proto.DECISION_ACCEPT_ALL, ()
        if self.config.accept == ACCEPT_FIRST:
            return proto.DECISION_ACCEPT_SOME, (offer.items[0].item_id, )
        return proto.DECISION_DECLINE, ()

    def _handle_events(self, events):
        for event in events:
            if isinstance(event, OfferReceived):
                decision, ids = self._decide(event.offer)
                self._handle_events(
                    self.session.respond(event.offer.transfer_id, decision,
                                         ids))
            elif isinstance(event, InlineItemReceived):
                self._save_whole(event.transfer_id, event.item,
                                 event.item.inline_data)
            elif isinstance(event, ChunkReceived):
                self._write_chunk(event)
            elif isinstance(event, ItemReceived):
                self._finish_item(event.transfer_id, event.item)
            elif isinstance(event, ResponseReceived):
                pass
            elif isinstance(event, TransferFinished):
                self._on_finished(event)

    def _record(self, transfer_id, item, saved_as=None, inline=None):
        self.report.received.append(
            ReceivedItem(transfer_id.hex(), item.item_id, item.kind, item.name,
                         item.size, item.sha256.hex(), saved_as, inline))

    def _save_whole(self, transfer_id, item, data):
        inline = data.decode('utf-8') if item.kind in (
            proto.ITEM_KIND_TEXT, proto.ITEM_KIND_URL) else None
        saved = None
        if self.config.receive_dir is not None:
            path = _unique_path(self.config.receive_dir,
                                sanitize_filename(item.name))
            path.write_bytes(data)
            saved = str(path)
        self._record(transfer_id, item, saved, inline)

    def _write_chunk(self, event):
        key = (event.transfer_id, event.item_id)
        if self.config.receive_dir is None:
            return
        if key not in self._open_files:
            path = _unique_path(
                self.config.receive_dir,
                sanitize_filename(self._item(event.transfer_id,
                                             event.item_id).name))
            part = path.with_name(path.name + '.part')
            self._open_files[key] = (path, part, part.open('wb'))
        self._open_files[key][2].write(event.data)

    def _item(self, transfer_id, item_id):
        offer = self.session.incoming.offer
        return next(i for i in offer.items if i.item_id == item_id)

    def _finish_item(self, transfer_id, item):
        key = (transfer_id, item.item_id)
        if key in self._open_files:
            path, part, f = self._open_files.pop(key)
            f.close()
            part.rename(path)
            self._record(transfer_id, item, str(path))
        elif item.size == 0:
            self._save_whole(transfer_id, item, b'')
        else:
            self._record(transfer_id, item)

    def _on_finished(self, event):
        for key in [k for k in self._open_files if k[0] == event.transfer_id]:
            _, part, f = self._open_files.pop(key)
            f.close()
            part.unlink()
        self.report.finished.append({
            'transfer_id': event.transfer_id.hex(),
            'outgoing': event.outgoing,
            'status': event.status,
            'cancel_reason': event.cancel_reason,
            'decision': event.decision,
        })
        if event.outgoing:
            self._outgoing_done = True
        else:
            self._incoming_finished += 1

    async def run(self, reader: asyncio.StreamReader,
                  writer: asyncio.StreamWriter) -> Report:
        c = self.config
        channel = await self._handshake(reader, writer)
        self.session = TransferSession(c.clock(), c.limits,
                                       send_confirmation=c.role == RESPONDER)
        wake = asyncio.Event()
        remote_closed = asyncio.Event()
        offer_sent = False

        async def read_loop():
            while True:
                message = await _read_message(reader)
                if message is None:
                    remote_closed.set()
                    wake.set()
                    return
                frame = channel.open(message)
                self._handle_events(
                    self.session.handle_frame(frame, c.clock()))
                wake.set()

        async def write_loop():
            nonlocal offer_sent
            while True:
                if remote_closed.is_set() and not self._work_done:
                    raise ProtocolError('remote closed before we finished')
                if channel.confirmed:
                    if not offer_sent and c.items:
                        self.session.send_offer(c.items, c.clock())
                        offer_sent = True
                    frame = self.session.next_frame(c.clock())
                    if frame is not None:
                        writer.write(channel.seal(frame))
                        await writer.drain()
                        continue
                    if self._work_done:
                        if writer.can_write_eof():
                            writer.write_eof()
                        else:
                            writer.close()
                        return
                wake.clear()
                try:
                    await asyncio.wait_for(wake.wait(), _POLL_INTERVAL)
                except asyncio.TimeoutError:
                    pass
                self._handle_events(self.session.poll(c.clock()))

        tasks = [
            asyncio.ensure_future(read_loop()),
            asyncio.ensure_future(write_loop())
        ]
        try:
            done, _ = await asyncio.wait(
                tasks, return_when=asyncio.FIRST_EXCEPTION)
            for task in done:
                task.result()
            await asyncio.gather(*tasks)
        finally:
            for task in tasks:
                task.cancel()
            for _, part, f in self._open_files.values():
                f.close()
                part.unlink()
        return self.report
