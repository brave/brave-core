# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Transfer state machine over an established SecureChannel (PROTOCOL.md §8).

Sans-IO: feed decoded frames to `handle_frame`, call `poll(now)` regularly,
and send whatever `next_frame()` returns until it returns None.
"""

import hashlib
import os
from collections import deque
from dataclasses import dataclass, field
from typing import Callable, List, Optional

from . import proto
from .channel import ProtocolError

TRANSFER_ID_LENGTH = 16
MAX_ITEMS = 256
MAX_NAME_BYTES = 1024
MAX_MIME_BYTES = 255
MAX_INLINE_BYTES = 8192
MAX_ITEM_SIZE = 1 << 40
MAX_CHUNK_DATA = 61440
OFFER_TIMEOUT = 60.0
IDLE_TIMEOUT = 30.0
PING_INTERVAL = 10.0


@dataclass
class Limits:
    max_items: int = MAX_ITEMS
    max_total_size: int = MAX_ITEM_SIZE


@dataclass
class OutgoingItem:
    kind: int
    name: str
    mime_type: str
    size: int
    sha256: bytes
    inline_data: bytes = b''
    # read(offset, length) -> bytes, for chunked items.
    read: Optional[Callable[[int, int], bytes]] = None

    @classmethod
    def from_bytes(cls, kind, name, mime_type, data: bytes):
        inline = kind in (proto.ITEM_KIND_TEXT, proto.ITEM_KIND_URL) and len(
            data) <= MAX_INLINE_BYTES
        return cls(kind, name, mime_type, len(data),
                   hashlib.sha256(data).digest(), data if inline else b'',
                   None if inline else
                   (lambda off, n: data[off:off + n]))


# Events returned to the embedder.
@dataclass
class OfferReceived:
    offer: proto.Offer


@dataclass
class ResponseReceived:
    transfer_id: bytes
    decision: int
    accepted_item_ids: List[int]


@dataclass
class InlineItemReceived:
    transfer_id: bytes
    item: proto.OfferItem


@dataclass
class ChunkReceived:
    transfer_id: bytes
    item_id: int
    offset: int
    data: bytes


@dataclass
class ItemReceived:
    transfer_id: bytes
    item: proto.OfferItem


@dataclass
class TransferFinished:
    transfer_id: bytes
    outgoing: bool
    # A DONE_STATUS_* value, or None when cancelled or declined.
    status: Optional[int]
    cancel_reason: Optional[int] = None
    decision: Optional[int] = None


def _utf8_len(s):
    return len(s.encode('utf-8'))


def validate_offer(offer: proto.Offer, limits: Limits):
    """Raises ProtocolError if the offer breaks PROTOCOL.md §8.2."""
    if len(offer.transfer_id) != TRANSFER_ID_LENGTH:
        raise ProtocolError('bad transfer id')
    if not 1 <= len(offer.items) <= limits.max_items:
        raise ProtocolError('bad item count')
    ids, total = set(), 0
    for item in offer.items:
        if item.item_id in ids:
            raise ProtocolError('duplicate item id')
        ids.add(item.item_id)
        if item.kind not in (proto.ITEM_KIND_FILE, proto.ITEM_KIND_IMAGE,
                             proto.ITEM_KIND_TEXT, proto.ITEM_KIND_URL):
            raise ProtocolError('bad item kind')
        if not 1 <= _utf8_len(item.name) <= MAX_NAME_BYTES:
            raise ProtocolError('bad item name')
        if _utf8_len(item.mime_type) > MAX_MIME_BYTES:
            raise ProtocolError('bad mime type')
        if len(item.sha256) != 32:
            raise ProtocolError('bad sha256')
        if item.size > MAX_ITEM_SIZE:
            raise ProtocolError('item too large')
        total += item.size
        textual = item.kind in (proto.ITEM_KIND_TEXT, proto.ITEM_KIND_URL)
        if textual and item.size == 0:
            raise ProtocolError('empty text item')
        must_inline = item.kind == proto.ITEM_KIND_URL or (
            textual and item.size <= MAX_INLINE_BYTES)
        if item.inline_data:
            if not must_inline or len(item.inline_data) != item.size:
                raise ProtocolError('bad inline data')
            if hashlib.sha256(item.inline_data).digest() != item.sha256:
                raise ProtocolError('inline data hash mismatch')
        elif must_inline:
            raise ProtocolError('missing inline data')
        if item.kind == proto.ITEM_KIND_URL:
            try:
                url = item.inline_data.decode('utf-8')
            except UnicodeDecodeError as e:
                raise ProtocolError('url is not UTF-8') from e
            if not url.lower().startswith(('http://', 'https://')):
                raise ProtocolError('url scheme not allowed')
        elif item.kind == proto.ITEM_KIND_TEXT and item.inline_data:
            try:
                item.inline_data.decode('utf-8')
            except UnicodeDecodeError as e:
                raise ProtocolError('text is not UTF-8') from e
    if total > limits.max_total_size:
        raise ProtocolError('offer too large')


class _Outgoing:

    def __init__(self, transfer_id, items, offered_at):
        self.transfer_id = transfer_id
        self.items = items  # list of (item_id, OutgoingItem)
        self.state = 'offered'  # offered -> sending -> awaiting_done
        self.offered_at = offered_at
        self.queue = deque()  # (item_id, OutgoingItem) still to stream
        self.offset = 0


class _Incoming:

    def __init__(self, offer, received_at):
        self.offer = offer
        self.state = 'pending'  # pending -> receiving
        self.received_at = received_at
        self.queue = deque()  # OfferItems still expected as chunks
        self.received = 0
        self.hasher = None


class TransferSession:

    def __init__(self,
                 now: float,
                 limits: Limits = None,
                 random=os.urandom,
                 send_confirmation=False):
        """`send_confirmation` must be set on the responder (PROTOCOL.md §4.3)."""
        self.limits = limits or Limits()
        self.random = random
        self.control = deque()
        self.outgoing: Optional[_Outgoing] = None
        self.incoming: Optional[_Incoming] = None
        self.finished_ids = set()
        self.last_received = now
        self.last_sent = now
        self.ping_sequence = 0
        if send_confirmation:
            self._queue(proto.Ping(sequence=0))

    # ----- Outgoing ---------------------------------------------------------

    def send_offer(self, items: List[OutgoingItem], now: float) -> bytes:
        if self.outgoing is not None:
            raise RuntimeError('an outgoing transfer is already active')
        transfer_id = self.random(TRANSFER_ID_LENGTH)
        numbered = list(enumerate(items, start=1))
        offer = proto.Offer(transfer_id=transfer_id,
                            items=[
                                proto.OfferItem(item_id=i,
                                                kind=it.kind,
                                                name=it.name,
                                                mime_type=it.mime_type,
                                                size=it.size,
                                                sha256=it.sha256,
                                                inline_data=it.inline_data)
                                for i, it in numbered
                            ])
        validate_offer(offer, Limits())
        self.outgoing = _Outgoing(transfer_id, numbered, now)
        self._queue(offer)
        return transfer_id

    def _start_sending(self, accepted_ids):
        out = self.outgoing
        out.state = 'sending'
        for item_id, item in out.items:
            if item_id in accepted_ids and not item.inline_data and item.size:
                out.queue.append((item_id, item))
        if not out.queue:
            out.state = 'awaiting_done'

    def _next_chunk(self) -> Optional[proto.Frame]:
        out = self.outgoing
        if out is None or out.state != 'sending':
            return None
        item_id, item = out.queue[0]
        n = min(MAX_CHUNK_DATA, item.size - out.offset)
        data = item.read(out.offset, n)
        if len(data) != n:
            raise RuntimeError('item source returned the wrong length')
        chunk = proto.Chunk(transfer_id=out.transfer_id,
                            item_id=item_id,
                            offset=out.offset,
                            data=data)
        out.offset += n
        if out.offset == item.size:
            out.queue.popleft()
            out.offset = 0
            if not out.queue:
                out.state = 'awaiting_done'
        return proto.Frame(chunk)

    # ----- Incoming ---------------------------------------------------------

    def respond(self, transfer_id: bytes, decision: int, accepted_ids=()):
        inc = self.incoming
        if inc is None or inc.offer.transfer_id != transfer_id or \
                inc.state != 'pending':
            raise RuntimeError('no pending offer with that id')
        offered = [i.item_id for i in inc.offer.items]
        if decision == proto.DECISION_ACCEPT_SOME:
            if not accepted_ids or not set(accepted_ids) <= set(offered):
                raise ValueError('bad accepted ids')
            accepted = set(accepted_ids)
        elif decision == proto.DECISION_ACCEPT_ALL:
            accepted_ids, accepted = (), set(offered)
        else:
            accepted_ids, accepted = (), set()
        self._queue(
            proto.Response(transfer_id=transfer_id,
                           decision=decision,
                           accepted_item_ids=sorted(accepted_ids)))
        events = []
        if not accepted:
            self._finish_incoming(events, None, decision=decision)
            return events
        inc.state = 'receiving'
        for item in inc.offer.items:
            if item.item_id not in accepted:
                continue
            if item.inline_data:
                events.append(InlineItemReceived(transfer_id, item))
            elif item.size == 0:
                events.append(ItemReceived(transfer_id, item))
            else:
                inc.queue.append(item)
        self._maybe_complete_incoming(events)
        return events

    def _maybe_complete_incoming(self, events):
        if not self.incoming.queue:
            self._queue(
                proto.Done(transfer_id=self.incoming.offer.transfer_id,
                           status=proto.DONE_STATUS_OK))
            self._finish_incoming(events, proto.DONE_STATUS_OK)

    def _finish_incoming(self, events, status, reason=None, decision=None):
        tid = self.incoming.offer.transfer_id
        self.finished_ids.add(tid)
        self.incoming = None
        events.append(TransferFinished(tid, False, status, reason, decision))

    def _finish_outgoing(self, events, status, reason=None, decision=None):
        tid = self.outgoing.transfer_id
        self.finished_ids.add(tid)
        self.outgoing = None
        events.append(TransferFinished(tid, True, status, reason, decision))

    def write_failed(self, transfer_id: bytes) -> list:
        """Ends the incoming transfer with Done{WRITE_FAILED}."""
        inc = self.incoming
        if inc is None or inc.offer.transfer_id != transfer_id or \
                inc.state != 'receiving':
            raise RuntimeError('no incoming transfer with that id')
        self._queue(
            proto.Done(transfer_id=transfer_id,
                       status=proto.DONE_STATUS_WRITE_FAILED))
        events = []
        self._finish_incoming(events, proto.DONE_STATUS_WRITE_FAILED)
        return events

    def cancel(self, transfer_id: bytes, reason=proto.CANCEL_REASON_USER):
        events = []
        self._queue(proto.Cancel(transfer_id=transfer_id, reason=reason))
        if self.outgoing and self.outgoing.transfer_id == transfer_id:
            self._finish_outgoing(events, None, reason)
        elif self.incoming and self.incoming.offer.transfer_id == transfer_id:
            self._finish_incoming(events, None, reason)
        return events

    # ----- Frames -----------------------------------------------------------

    def _queue(self, body):
        self.control.append(proto.Frame(body))

    def next_frame(self, now: float) -> Optional[proto.Frame]:
        frame = self.control.popleft() if self.control else self._next_chunk()
        if frame is not None:
            self.last_sent = now
        return frame

    def handle_frame(self, frame: proto.Frame, now: float) -> list:
        self.last_received = now
        body = frame.body
        events = []
        if isinstance(body, proto.Ping):
            pass
        elif isinstance(body, proto.Offer):
            self._on_offer(body, now, events)
        elif isinstance(body, proto.Response):
            self._on_response(body, events)
        elif isinstance(body, proto.Chunk):
            self._on_chunk(body, events)
        elif isinstance(body, proto.Done):
            self._on_done(body, events)
        elif isinstance(body, proto.Cancel):
            self._on_cancel(body, events)
        return events

    def _on_offer(self, offer, now, events):
        validate_offer(offer, self.limits)
        if offer.transfer_id in self.finished_ids or (
                self.outgoing
                and self.outgoing.transfer_id == offer.transfer_id):
            raise ProtocolError('reused transfer id')
        if self.incoming is not None:
            if self.incoming.offer.transfer_id == offer.transfer_id:
                raise ProtocolError('reused transfer id')
            self._queue(
                proto.Response(transfer_id=offer.transfer_id,
                               decision=proto.DECISION_BUSY))
            self.finished_ids.add(offer.transfer_id)
            return
        self.incoming = _Incoming(offer, now)
        events.append(OfferReceived(offer))

    def _on_response(self, resp, events):
        out = self.outgoing
        if out is None or out.transfer_id != resp.transfer_id:
            if resp.transfer_id in self.finished_ids:
                return  # raced with a cancel or timeout
            raise ProtocolError('response for unknown transfer')
        if out.state != 'offered':
            raise ProtocolError('duplicate response')
        offered = {i for i, _ in out.items}
        if resp.decision == proto.DECISION_ACCEPT_ALL:
            if resp.accepted_item_ids:
                raise ProtocolError('ACCEPT_ALL with item ids')
            accepted = offered
        elif resp.decision == proto.DECISION_ACCEPT_SOME:
            accepted = set(resp.accepted_item_ids)
            if not accepted or len(accepted) != len(
                    resp.accepted_item_ids) or not accepted <= offered:
                raise ProtocolError('bad accepted item ids')
        elif resp.decision in (proto.DECISION_DECLINE, proto.DECISION_BUSY,
                               proto.DECISION_NO_SPACE):
            if resp.accepted_item_ids:
                raise ProtocolError('item ids on a refusal')
            events.append(
                ResponseReceived(resp.transfer_id, resp.decision, []))
            self._finish_outgoing(events, None, decision=resp.decision)
            return
        else:
            raise ProtocolError('unknown decision')
        events.append(
            ResponseReceived(resp.transfer_id, resp.decision,
                             sorted(accepted)))
        self._start_sending(accepted)

    def _on_chunk(self, chunk, events):
        inc = self.incoming
        if inc is None or inc.offer.transfer_id != chunk.transfer_id:
            if chunk.transfer_id in self.finished_ids:
                return  # in flight when we cancelled
            raise ProtocolError('chunk for unknown transfer')
        if inc.state != 'receiving':
            raise ProtocolError('chunk before response')
        item = inc.queue[0]
        if chunk.item_id != item.item_id or chunk.offset != inc.received:
            raise ProtocolError('out-of-order chunk')
        if not 1 <= len(chunk.data) <= MAX_CHUNK_DATA or \
                chunk.offset + len(chunk.data) > item.size:
            raise ProtocolError('bad chunk length')
        if inc.hasher is None:
            inc.hasher = hashlib.sha256()
        inc.hasher.update(chunk.data)
        inc.received += len(chunk.data)
        events.append(
            ChunkReceived(chunk.transfer_id, chunk.item_id, chunk.offset,
                          chunk.data))
        if inc.received < item.size:
            return
        ok = inc.hasher.digest() == item.sha256
        inc.queue.popleft()
        inc.received, inc.hasher = 0, None
        if not ok:
            self._queue(
                proto.Done(transfer_id=chunk.transfer_id,
                           status=proto.DONE_STATUS_HASH_MISMATCH))
            self._finish_incoming(events, proto.DONE_STATUS_HASH_MISMATCH)
            return
        events.append(ItemReceived(chunk.transfer_id, item))
        self._maybe_complete_incoming(events)

    def _on_done(self, done, events):
        out = self.outgoing
        if out is None or out.transfer_id != done.transfer_id:
            if done.transfer_id in self.finished_ids:
                return
            raise ProtocolError('done for unknown transfer')
        if out.state == 'offered':
            raise ProtocolError('done before response')
        if done.status == proto.DONE_STATUS_OK and out.state != 'awaiting_done':
            raise ProtocolError('done OK before all chunks were sent')
        self._finish_outgoing(events, done.status)

    def _on_cancel(self, cancel, events):
        if self.outgoing and self.outgoing.transfer_id == cancel.transfer_id:
            self._finish_outgoing(events, None, cancel.reason)
        elif self.incoming and \
                self.incoming.offer.transfer_id == cancel.transfer_id:
            self._finish_incoming(events, None, cancel.reason)
        # Unknown ids are ignored: the transfer may already have ended.

    # ----- Timers -----------------------------------------------------------

    def poll(self, now: float) -> list:
        """Runs timers. Raises ProtocolError on idle timeout."""
        events = []
        if now - self.last_received > IDLE_TIMEOUT:
            raise ProtocolError('idle timeout')
        out = self.outgoing
        if out and out.state == 'offered' and \
                now - out.offered_at > OFFER_TIMEOUT:
            events += self.cancel(out.transfer_id, proto.CANCEL_REASON_TIMEOUT)
        inc = self.incoming
        if inc and inc.state == 'pending' and \
                now - inc.received_at > OFFER_TIMEOUT:
            events += self.respond(inc.offer.transfer_id,
                                   proto.DECISION_DECLINE)
        if now - self.last_sent >= PING_INTERVAL and not self.control:
            self.ping_sequence += 1
            self._queue(proto.Ping(sequence=self.ping_sequence))
        return events

    @property
    def idle(self) -> bool:
        return self.outgoing is None and self.incoming is None and \
            not self.control
