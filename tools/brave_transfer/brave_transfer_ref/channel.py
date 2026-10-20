# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Connection setup and framing (PROTOCOL.md §4 and §6), without I/O.

Every message on the TCP stream, handshake or transport, is a 2-byte
big-endian length followed by that many bytes.
"""

import struct

from . import PROTOCOL_VERSION
from . import proto
from .keys import TAG_LENGTH
from .noise import TAGLEN, HandshakeState, NoiseError

PROLOGUE = b'brave-transfer/1'
MAX_WIRE_MESSAGE = 65535
MAX_FRAME_PLAINTEXT = MAX_WIRE_MESSAGE - TAGLEN
MAX_CONTROL_FRAME = 16384
MAX_DEVICE_NAME_BYTES = 128
MAX_GUID_BYTES = 128


class ProtocolError(Exception):
    pass


def length_prefix(message: bytes) -> bytes:
    if len(message) > MAX_WIRE_MESSAGE:
        raise ProtocolError('message too long')
    return struct.pack('>H', len(message)) + message


class MessageReader:
    """Splits a byte stream into length-prefixed messages."""

    def __init__(self):
        self.buffer = bytearray()

    def feed(self, data: bytes):
        self.buffer += data
        out = []
        while len(self.buffer) >= 2:
            n = struct.unpack('>H', self.buffer[:2])[0]
            if len(self.buffer) < 2 + n:
                break
            out.append(bytes(self.buffer[2:2 + n]))
            del self.buffer[:2 + n]
        return out


def validate_peer_info(info: proto.PeerInfo):
    if info.protocol_version != PROTOCOL_VERSION:
        raise ProtocolError('unsupported protocol version')
    if len(info.device_name.encode()) > MAX_DEVICE_NAME_BYTES:
        raise ProtocolError('device name too long')
    if len(info.sync_device_guid.encode()) > MAX_GUID_BYTES:
        raise ProtocolError('guid too long')


class SecureChannel:
    """Transport phase: encrypts and decrypts Frames."""

    def __init__(self, handshake: HandshakeState, remote_info: proto.PeerInfo):
        self.send_cipher = handshake.send_cipher
        self.recv_cipher = handshake.recv_cipher
        self.handshake_hash = handshake.handshake_hash
        self.remote_static = handshake.rs
        self.remote_info = remote_info
        # The initiator only knows the responder holds the PSK once a
        # transport message from it decrypts (PROTOCOL.md §4.3).
        self.confirmed = not handshake.initiator

    def seal(self, frame: proto.Frame) -> bytes:
        plaintext = frame.encode()
        limit = MAX_FRAME_PLAINTEXT if isinstance(
            frame.body, proto.Chunk) else MAX_CONTROL_FRAME
        if len(plaintext) > limit:
            raise ProtocolError('frame too large')
        return length_prefix(self.send_cipher.encrypt(b'', plaintext))

    def open(self, message: bytes) -> proto.Frame:
        try:
            plaintext = self.recv_cipher.decrypt(b'', message)
        except NoiseError as e:
            raise ProtocolError('transport decryption failed') from e
        self.confirmed = True
        try:
            frame = proto.Frame.decode(plaintext)
        except proto.DecodeError as e:
            raise ProtocolError(f'bad frame: {e}') from e
        if not isinstance(frame.body,
                          proto.Chunk) and len(plaintext) > MAX_CONTROL_FRAME:
            raise ProtocolError('control frame too large')
        return frame


class InitiatorHandshake:
    """Desktop side: dials after seeing a matching discovery tag."""

    def __init__(self, static_private, psk, trust_tag, local_info,
                 ephemeral_private=None):
        if len(trust_tag) != TAG_LENGTH:
            raise ValueError('bad tag')
        self.local_info = local_info
        self.trust_tag = trust_tag
        self.hs = HandshakeState(True, PROLOGUE, static_private, psk,
                                 ephemeral_private)
        self.remote_info = None

    def message1(self) -> bytes:
        hello = proto.ClientHello(protocol_version=PROTOCOL_VERSION,
                                  trust_tag=self.trust_tag)
        return self.hs.write_message(hello.encode())

    def read_message2(self, message: bytes) -> proto.PeerInfo:
        try:
            info = proto.PeerInfo.decode(self.hs.read_message(message))
        except (NoiseError, proto.DecodeError) as e:
            raise ProtocolError(f'bad handshake message 2: {e}') from e
        validate_peer_info(info)
        self.remote_info = info
        return info

    def message3(self) -> bytes:
        return self.hs.write_message(self.local_info.encode())

    def channel(self) -> SecureChannel:
        return SecureChannel(self.hs, self.remote_info)


class ResponderHandshake:
    """iPhone side: listens and advertises tags.

    `psk_for_tag(tag)` returns the PSK for a tag it accepts, or None.
    """

    def __init__(self, static_private, psk_for_tag, local_info,
                 ephemeral_private=None):
        self.psk_for_tag = psk_for_tag
        self.local_info = local_info
        self.hs = HandshakeState(False, PROLOGUE, static_private, None,
                                 ephemeral_private)
        self.client_hello = None
        self.remote_info = None

    def read_message1(self, message: bytes) -> proto.ClientHello:
        try:
            hello = proto.ClientHello.decode(self.hs.read_message(message))
        except (NoiseError, proto.DecodeError) as e:
            raise ProtocolError(f'bad handshake message 1: {e}') from e
        if hello.protocol_version != PROTOCOL_VERSION:
            raise ProtocolError('unsupported protocol version')
        psk = self.psk_for_tag(hello.trust_tag)
        if psk is None:
            raise ProtocolError('unknown trust tag')
        self.hs.set_psk(psk)
        self.client_hello = hello
        return hello

    def message2(self) -> bytes:
        return self.hs.write_message(self.local_info.encode())

    def read_message3(self, message: bytes) -> proto.PeerInfo:
        try:
            info = proto.PeerInfo.decode(self.hs.read_message(message))
        except (NoiseError, proto.DecodeError) as e:
            raise ProtocolError(f'bad handshake message 3: {e}') from e
        validate_peer_info(info)
        self.remote_info = info
        return info

    def channel(self) -> SecureChannel:
        return SecureChannel(self.hs, self.remote_info)
