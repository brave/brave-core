# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Hand-written codec for brave_transfer.proto (PROTOCOL.md §5).

Encoding is canonical: fields in ascending number order, proto3 default
values omitted, `accepted_item_ids` packed, the Frame body always present.
tests/test_proto_conformance.py checks it against the protobuf runtime.
"""

from dataclasses import dataclass, field, fields
from typing import List, Optional

# Enum values (see brave_transfer.proto).
DEVICE_TYPE_DESKTOP, DEVICE_TYPE_PHONE, DEVICE_TYPE_TABLET = 1, 2, 3
ITEM_KIND_FILE, ITEM_KIND_IMAGE, ITEM_KIND_TEXT, ITEM_KIND_URL = 1, 2, 3, 4
(DECISION_ACCEPT_ALL, DECISION_ACCEPT_SOME, DECISION_DECLINE, DECISION_BUSY,
 DECISION_NO_SPACE) = 1, 2, 3, 4, 5
CANCEL_REASON_USER, CANCEL_REASON_TIMEOUT, CANCEL_REASON_ERROR = 1, 2, 3
(DONE_STATUS_OK, DONE_STATUS_HASH_MISMATCH, DONE_STATUS_WRITE_FAILED,
 DONE_STATUS_INTERRUPTED) = 1, 2, 3, 4

CAPABILITY_RECEIVE = 1 << 0
CAPABILITY_SEND = 1 << 1

VARINT, I64, LEN, I32 = 0, 1, 2, 5


class DecodeError(ValueError):
    pass


def encode_varint(value: int) -> bytes:
    if value < 0 or value >= 1 << 64:
        raise ValueError('varint out of range')
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return bytes(out)


class Reader:

    def __init__(self, data: bytes):
        self.data = memoryview(data)
        self.pos = 0

    def at_end(self):
        return self.pos >= len(self.data)

    def varint(self) -> int:
        """Like protobuf, bits beyond 64 in a 10-byte varint are dropped."""
        result = 0
        for i in range(10):
            if self.pos >= len(self.data):
                raise DecodeError('truncated varint')
            byte = self.data[self.pos]
            self.pos += 1
            result |= (byte & 0x7F) << (7 * i)
            if not byte & 0x80:
                return result & 0xFFFFFFFFFFFFFFFF
        raise DecodeError('varint too long')

    def take(self, n: int) -> bytes:
        if n > len(self.data) - self.pos:
            raise DecodeError('truncated field')
        out = bytes(self.data[self.pos:self.pos + n])
        self.pos += n
        return out

    def fields(self):
        """Yields (field_number, wire_type, value). LEN values are bytes."""
        while not self.at_end():
            key = self.varint()
            number, wire_type = key >> 3, key & 7
            if number == 0 or number >= 1 << 29:
                raise DecodeError('bad field number')
            if wire_type == VARINT:
                value = self.varint()
            elif wire_type == I64:
                value = self.take(8)
            elif wire_type == LEN:
                value = self.take(self.varint())
            elif wire_type == I32:
                value = self.take(4)
            else:
                raise DecodeError('unsupported wire type')
            yield number, wire_type, value


# Field schema: (number, attribute, kind[, message class]).
# kinds: u32, u64 (varint), str, bytes, msg, rep_msg, packed_u32.


def _encode(msg) -> bytes:
    out = bytearray()
    for number, name, kind, *cls in msg.SCHEMA:
        value = getattr(msg, name)
        if kind in ('u32', 'u64'):
            if value:
                out += encode_varint(number << 3 | VARINT) + encode_varint(
                    value)
        elif kind in ('str', 'bytes'):
            if value:
                raw = value.encode('utf-8') if kind == 'str' else value
                out += encode_varint(number << 3 | LEN) + encode_varint(
                    len(raw)) + raw
        elif kind == 'rep_msg':
            for item in value:
                raw = item.encode()
                out += encode_varint(number << 3 | LEN) + encode_varint(
                    len(raw)) + raw
        elif kind == 'packed_u32':
            if value:
                raw = b''.join(encode_varint(v) for v in value)
                out += encode_varint(number << 3 | LEN) + encode_varint(
                    len(raw)) + raw
        else:
            raise AssertionError(kind)
    return bytes(out)


def _decode(cls, data: bytes):
    schema = {s[0]: s for s in cls.SCHEMA}
    values = {}
    for number, wire_type, value in Reader(data).fields():
        spec = schema.get(number)
        if spec is None:
            continue
        _, name, kind, *sub = spec
        if kind in ('u32', 'u64') and wire_type == VARINT:
            values[name] = value & 0xFFFFFFFF if kind == 'u32' else value
        elif kind == 'str' and wire_type == LEN:
            try:
                values[name] = value.decode('utf-8')
            except UnicodeDecodeError as e:
                raise DecodeError('invalid UTF-8') from e
        elif kind == 'bytes' and wire_type == LEN:
            values[name] = value
        elif kind == 'rep_msg' and wire_type == LEN:
            values.setdefault(name, []).append(sub[0].decode(value))
        elif kind == 'packed_u32' and wire_type == LEN:
            r = Reader(value)
            while not r.at_end():
                values.setdefault(name, []).append(r.varint() & 0xFFFFFFFF)
        elif kind == 'packed_u32' and wire_type == VARINT:
            values.setdefault(name, []).append(value & 0xFFFFFFFF)
        # Known number with the wrong wire type: skipped, like protobuf.
    return cls(**values)


class _Message:
    SCHEMA = ()

    def encode(self) -> bytes:
        return _encode(self)

    @classmethod
    def decode(cls, data: bytes):
        return _decode(cls, data)

    def to_json(self):
        """Test-vector form: bytes as hex, defaults included."""
        out = {}
        for f in fields(self):
            v = getattr(self, f.name)
            if isinstance(v, bytes):
                v = v.hex()
            elif isinstance(v, list):
                v = [x.to_json() if isinstance(x, _Message) else x for x in v]
            out[f.name] = v
        return out

    @classmethod
    def from_json(cls, value: dict):
        """Inverse of to_json; missing keys take their default."""
        values = {}
        for _, name, kind, *sub in cls.SCHEMA:
            if name not in value:
                continue
            v = value[name]
            if kind == 'bytes':
                v = bytes.fromhex(v)
            elif kind == 'rep_msg':
                v = [sub[0].from_json(x) for x in v]
            values[name] = v
        return cls(**values)


@dataclass
class ClientHello(_Message):
    protocol_version: int = 0
    trust_tag: bytes = b''
    SCHEMA = ((1, 'protocol_version', 'u32'), (2, 'trust_tag', 'bytes'))


@dataclass
class PeerInfo(_Message):
    protocol_version: int = 0
    device_name: str = ''
    device_type: int = 0
    sync_device_guid: str = ''
    capabilities: int = 0
    SCHEMA = ((1, 'protocol_version', 'u32'), (2, 'device_name', 'str'),
              (3, 'device_type', 'u32'), (4, 'sync_device_guid', 'str'),
              (5, 'capabilities', 'u64'))


@dataclass
class OfferItem(_Message):
    item_id: int = 0
    kind: int = 0
    name: str = ''
    mime_type: str = ''
    size: int = 0
    sha256: bytes = b''
    inline_data: bytes = b''
    SCHEMA = ((1, 'item_id', 'u32'), (2, 'kind', 'u32'), (3, 'name', 'str'),
              (4, 'mime_type', 'str'), (5, 'size', 'u64'),
              (6, 'sha256', 'bytes'), (7, 'inline_data', 'bytes'))


@dataclass
class Offer(_Message):
    transfer_id: bytes = b''
    items: List[OfferItem] = field(default_factory=list)
    SCHEMA = ((1, 'transfer_id', 'bytes'), (2, 'items', 'rep_msg', OfferItem))


@dataclass
class Response(_Message):
    transfer_id: bytes = b''
    decision: int = 0
    accepted_item_ids: List[int] = field(default_factory=list)
    SCHEMA = ((1, 'transfer_id', 'bytes'), (2, 'decision', 'u32'),
              (3, 'accepted_item_ids', 'packed_u32'))


@dataclass
class Cancel(_Message):
    transfer_id: bytes = b''
    reason: int = 0
    SCHEMA = ((1, 'transfer_id', 'bytes'), (2, 'reason', 'u32'))


@dataclass
class Done(_Message):
    transfer_id: bytes = b''
    status: int = 0
    SCHEMA = ((1, 'transfer_id', 'bytes'), (2, 'status', 'u32'))


@dataclass
class Ping(_Message):
    sequence: int = 0
    SCHEMA = ((1, 'sequence', 'u64'), )


@dataclass
class Chunk(_Message):
    transfer_id: bytes = b''
    item_id: int = 0
    offset: int = 0
    data: bytes = b''
    SCHEMA = ((1, 'transfer_id', 'bytes'), (2, 'item_id', 'u32'),
              (3, 'offset', 'u64'), (4, 'data', 'bytes'))


FRAME_BODIES = {1: Offer, 2: Response, 3: Cancel, 4: Done, 5: Ping, 6: Chunk}
_BODY_NUMBERS = {cls: n for n, cls in FRAME_BODIES.items()}


@dataclass
class Frame:
    body: Optional[_Message] = None

    def encode(self) -> bytes:
        if self.body is None:
            raise ValueError('frame without body')
        raw = self.body.encode()
        return encode_varint(_BODY_NUMBERS[type(self.body)] << 3
                             | LEN) + encode_varint(len(raw)) + raw

    @classmethod
    def decode(cls, data: bytes) -> 'Frame':
        body = None
        for number, wire_type, value in Reader(data).fields():
            if number in FRAME_BODIES and wire_type == LEN:
                if body is not None:
                    raise DecodeError('frame has more than one body')
                body = FRAME_BODIES[number].decode(value)
        if body is None:
            raise DecodeError('frame has no body')
        return cls(body)

    def to_json(self):
        return {_BODY_NAMES[type(self.body)]: self.body.to_json()}

    @classmethod
    def from_json(cls, value: dict) -> 'Frame':
        (name, body), = value.items()
        return cls(_BODY_CLASSES[name].from_json(body))


MESSAGES = {
    c.__name__: c
    for c in (ClientHello, PeerInfo, OfferItem, Offer, Response, Cancel, Done,
              Ping, Chunk, Frame)
}
_BODY_CLASSES = {
    'offer': Offer,
    'response': Response,
    'cancel': Cancel,
    'done': Done,
    'ping': Ping,
    'chunk': Chunk
}
_BODY_NAMES = {cls: name for name, cls in _BODY_CLASSES.items()}
