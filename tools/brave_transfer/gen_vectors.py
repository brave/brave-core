#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Writes the Brave Transfer test vectors in components/brave_transfer/core/test/data.

Everything is deterministic. Run with --check to verify the committed files
are up to date (tests/test_vectors.py does this).
"""

import argparse
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

# pylint: disable=wrong-import-position
from brave_transfer_ref import PROTOCOL_VERSION, bip39, proto
from brave_transfer_ref.channel import (PROLOGUE, InitiatorHandshake,
                                        ProtocolError, ResponderHandshake)
from brave_transfer_ref.keys import (EPOCH_SECONDS, ChainKeys, epoch_for,
                                     fingerprint, x25519_public)
from brave_transfer_ref.noise import PROTOCOL_NAME
from brave_transfer_ref.sanitize import sanitize_filename
from brave_transfer_ref.session import (Limits, OfferReceived, OutgoingItem,
                                        TransferFinished, TransferSession,
                                        validate_offer)

DATA_DIR = Path(__file__).resolve().parents[2] / 'components' / \
    'brave_transfer' / 'core' / 'test' / 'data'
CACOPHONY_SOURCE = (
    'https://github.com/mcginty/snow/blob/main/tests/vectors/cacophony.txt')

# Transcript scenarios start at this time; any value works.
SESSION_START = 1000.0
# 2026-10-15T08:53:20Z, used where a realistic wall-clock time helps.
REFERENCE_TIME = 1792054400


def labelled(label: str, n=32) -> bytes:
    """Deterministic test bytes."""
    return hashlib.sha256(b'brave-transfer test ' + label.encode()).digest()[:n]


def pattern(n: int) -> bytes:
    """Chunked item contents in the vectors: byte i is i % 251."""
    return bytes(i % 251 for i in range(n))


# ----- key_derivation.json -------------------------------------------------


def key_derivation():
    seeds = [
        ('all zero entropy', bytes(32)),
        ('all 0xff entropy', b'\xff' * 32),
        ('counting bytes', bytes(range(32))),
        ('labelled', labelled('seed')),
    ]
    valid = []
    for name, seed in seeds:
        code = bip39.bytes32_to_mnemonic(seed)
        valid.append(_key_entry(name, code, seed))
    brave_unittest_code = (
        'magic vacuum wide review love peace century egg burden clutch heart '
        'cycle annual mixed pink awesome extra client cry brisk priority maple '
        'mountain jelly')
    valid.append(
        _key_entry('brave_sync crypto_unittest passphrase', brave_unittest_code,
                   bip39.mnemonic_to_bytes32(brave_unittest_code)))

    good = valid[3]['sync_code']
    words = good.split(' ')
    invalid = [
        ('empty', ''),
        ('23 words', ' '.join(words[:23])),
        ('25 words (time-limited code format)', good + ' abandon'),
        ('12-word BIP39 code', ' '.join(['abandon'] * 11 + ['about'])),
        ('bad checksum', ' '.join(['abandon'] * 24)),
        ('brave_sync crypto_unittest invalid passphrase',
         brave_unittest_code.rsplit(' ', 1)[0] + ' brave'),
        ('unknown word', ' '.join(words[:23] + ['bravely'])),
        ('upper case', good.upper()),
        ('double space', good.replace(' ', '  ', 1)),
        ('leading space', ' ' + good),
        ('trailing newline', good + '\n'),
    ]
    for _, code in invalid:
        try:
            bip39.mnemonic_to_bytes32(code)
        except bip39.InvalidMnemonic:
            continue
        raise AssertionError(f'expected {code!r} to be rejected')
    return {
        'description':
        'seed = PassphraseToBytes32(sync_code) (BIP39 English, 24 words); '
        'root = HKDF-SHA256(salt="brave-transfer", ikm=seed, info="v1/root"); '
        'psk = HKDF-SHA256(salt="", ikm=root, info="v1/chain-psk"); '
        'discovery = HKDF-SHA256(salt="", ikm=root, info="v1/discovery"). '
        'See PROTOCOL.md section 3.',
        'valid': valid,
        'invalid_sync_codes': [{
            'name': n,
            'sync_code': c
        } for n, c in invalid],
    }


def _key_entry(name, code, seed):
    keys = ChainKeys.from_seed(seed)
    return {
        'name': name,
        'sync_code': code,
        'seed': seed.hex(),
        'root_key': keys.root.hex(),
        'psk': keys.psk.hex(),
        'discovery_key': keys.discovery.hex(),
    }


# ----- discovery_tags.json -------------------------------------------------


def discovery_tags():
    seed = labelled('seed')
    keys = ChainKeys.from_seed(seed)
    times = [
        0, EPOCH_SECONDS - 1, EPOCH_SECONDS, REFERENCE_TIME - 1,
        REFERENCE_TIME, REFERENCE_TIME + EPOCH_SECONDS - 1,
        REFERENCE_TIME + EPOCH_SECONDS, 4102444800
    ]
    return {
        'description':
        'epoch = floor(unix_time / 900); tag = HMAC-SHA256(discovery_key, '
        'uint64_be(epoch))[0:8]; TXT c = lowercase hex(tag). A responder '
        'accepts tags for epochs t-1, t and t+1. See PROTOCOL.md section 3.3.',
        'seed': seed.hex(),
        'discovery_key': keys.discovery.hex(),
        'tags': [{
            'unix_time': t,
            'epoch': epoch_for(t),
            'tag': keys.tag(epoch_for(t)).hex(),
        } for t in times],
        'accepted': [{
            'unix_time': t,
            'tags': [x.hex() for x in keys.accepted_tags(t)],
        } for t in (0, REFERENCE_TIME, REFERENCE_TIME + 450)],
        'txt_record': {
            'unix_time': REFERENCE_TIME,
            'entries': ['v=1', f'c={keys.tag(epoch_for(REFERENCE_TIME)).hex()}'],
        },
    }


# ----- noise_handshake.json ------------------------------------------------


def _peer_info(name, device_type, guid):
    return proto.PeerInfo(protocol_version=PROTOCOL_VERSION,
                          device_name=name,
                          device_type=device_type,
                          sync_device_guid=guid,
                          capabilities=proto.CAPABILITY_RECEIVE
                          | proto.CAPABILITY_SEND)


def noise_handshake():
    keys = ChainKeys.from_seed(labelled('seed'))
    tag = keys.tag(epoch_for(REFERENCE_TIME))
    i_static, r_static = labelled('initiator static'), labelled(
        'responder static')
    i_eph, r_eph = labelled('initiator ephemeral'), labelled(
        'responder ephemeral')
    i_info = _peer_info('MacBook Pro', proto.DEVICE_TYPE_DESKTOP,
                        'f0a1b2c3-0000-4000-8000-000000000001')
    r_info = _peer_info("James's iPhone", proto.DEVICE_TYPE_PHONE,
                        'f0a1b2c3-0000-4000-8000-000000000002')

    init = InitiatorHandshake(i_static, keys.psk, tag, i_info, i_eph)
    resp = ResponderHandshake(r_static, lambda t: keys.psk
                              if t == tag else None, r_info, r_eph)
    m1 = init.message1()
    resp.read_message1(m1)
    m2 = resp.message2()
    init.read_message2(m2)
    m3 = init.message3()
    resp.read_message3(m3)
    ic, rc = init.channel(), resp.channel()
    assert ic.handshake_hash == rc.handshake_hash

    text = b'Hello from Brave Transfer'
    transport = [
        ('responder', rc, proto.Frame(proto.Ping(sequence=0))),
        ('initiator', ic,
         proto.Frame(
             proto.Offer(transfer_id=labelled('transfer id', 16),
                         items=[
                             proto.OfferItem(item_id=1,
                                             kind=proto.ITEM_KIND_TEXT,
                                             name='text.txt',
                                             mime_type='text/plain',
                                             size=len(text),
                                             sha256=hashlib.sha256(
                                                 text).digest(),
                                             inline_data=text)
                         ]))),
        ('responder', rc,
         proto.Frame(
             proto.Response(transfer_id=labelled('transfer id', 16),
                            decision=proto.DECISION_ACCEPT_ALL))),
    ]
    transport_out = []
    for sender, ch, frame in transport:
        wire = ch.seal(frame)
        other = rc if ch is ic else ic
        assert other.open(wire[2:]) == frame
        transport_out.append({
            'sender': sender,
            'frame': frame.to_json(),
            'plaintext': frame.encode().hex(),
            'wire': wire.hex(),
        })

    def msg(sender, payload, wire):
        return {
            'sender': sender,
            'payload_message': payload.to_json(),
            'payload': payload.encode().hex(),
            'message': wire.hex(),
        }

    return {
        'description':
        'One complete Brave Transfer connection: the XXpsk3 handshake with '
        'the Brave prologue and payloads, then three transport messages. '
        '"message" is the Noise message; "wire" adds the 2-byte big-endian '
        'length prefix. See PROTOCOL.md sections 4 and 6.',
        'protocol_name': PROTOCOL_NAME.decode(),
        'prologue': PROLOGUE.hex(),
        'unix_time': REFERENCE_TIME,
        'seed': labelled('seed').hex(),
        'psk': keys.psk.hex(),
        'trust_tag': tag.hex(),
        'initiator_static': i_static.hex(),
        'initiator_static_public': x25519_public(i_static).hex(),
        'initiator_fingerprint': fingerprint(x25519_public(i_static)),
        'initiator_ephemeral': i_eph.hex(),
        'responder_static': r_static.hex(),
        'responder_static_public': x25519_public(r_static).hex(),
        'responder_fingerprint': fingerprint(x25519_public(r_static)),
        'responder_ephemeral': r_eph.hex(),
        'handshake_messages': [
            msg('initiator',
                proto.ClientHello(protocol_version=PROTOCOL_VERSION,
                                  trust_tag=tag), m1),
            msg('responder', r_info, m2),
            msg('initiator', i_info, m3),
        ],
        'handshake_hash': ic.handshake_hash.hex(),
        'initiator_to_responder_key': ic.send_cipher.key.hex(),
        'responder_to_initiator_key': rc.send_cipher.key.hex(),
        'transport_messages': transport_out,
    }


def cacophony(source: Path):
    vectors = json.loads(source.read_text())['vectors']
    (v, ) = [
        x for x in vectors if x.get('protocol_name') == PROTOCOL_NAME.decode()
    ]
    return {
        'description':
        'The Noise_XXpsk3_25519_ChaChaPoly_SHA256 vector from cacophony. '
        'Messages 4-6 are transport messages after Split(). Payloads are '
        'not Brave Transfer messages.',
        'source': CACOPHONY_SOURCE,
        **v,
    }


# ----- proto_messages.json -------------------------------------------------


def _offer_example():
    text = 'Hello from Brave Transfer'.encode()
    return proto.Offer(
        transfer_id=labelled('transfer id', 16),
        items=[
            proto.OfferItem(item_id=1,
                            kind=proto.ITEM_KIND_TEXT,
                            name='text.txt',
                            mime_type='text/plain',
                            size=len(text),
                            sha256=hashlib.sha256(text).digest(),
                            inline_data=text),
            proto.OfferItem(item_id=2,
                            kind=proto.ITEM_KIND_IMAGE,
                            name='Café photo 📷.jpg',
                            mime_type='image/jpeg',
                            size=5_000_000_000,
                            sha256=labelled('photo')),
        ])


def proto_messages():
    tid = labelled('transfer id', 16)
    encode = [
        ('empty ClientHello', proto.ClientHello()),
        ('ClientHello', proto.ClientHello(protocol_version=1,
                                          trust_tag=labelled('tag', 8))),
        ('PeerInfo', _peer_info("James's iPhone", proto.DEVICE_TYPE_PHONE,
                                'f0a1b2c3-0000-4000-8000-000000000002')),
        ('PeerInfo with large capabilities',
         proto.PeerInfo(protocol_version=1, capabilities=1 << 40)),
        ('OfferItem', _offer_example().items[1]),
        ('Offer', _offer_example()),
        ('Response ACCEPT_ALL',
         proto.Response(transfer_id=tid, decision=proto.DECISION_ACCEPT_ALL)),
        ('Response ACCEPT_SOME',
         proto.Response(transfer_id=tid,
                        decision=proto.DECISION_ACCEPT_SOME,
                        accepted_item_ids=[1, 3, 200, 70000])),
        ('Cancel', proto.Cancel(transfer_id=tid,
                                reason=proto.CANCEL_REASON_TIMEOUT)),
        ('Done', proto.Done(transfer_id=tid, status=proto.DONE_STATUS_OK)),
        ('Ping 0', proto.Ping()),
        ('Ping', proto.Ping(sequence=300)),
        ('Chunk', proto.Chunk(transfer_id=tid, item_id=2, offset=61440,
                              data=pattern(40))),
        ('Frame with empty Ping', proto.Frame(proto.Ping())),
        ('Frame with Ping', proto.Frame(proto.Ping(sequence=7))),
        ('Frame with Offer', proto.Frame(_offer_example())),
        ('Frame with Response',
         proto.Frame(proto.Response(transfer_id=tid,
                                    decision=proto.DECISION_DECLINE))),
        ('Frame with Cancel',
         proto.Frame(proto.Cancel(transfer_id=tid,
                                  reason=proto.CANCEL_REASON_USER))),
        ('Frame with Done',
         proto.Frame(proto.Done(transfer_id=tid,
                                status=proto.DONE_STATUS_HASH_MISMATCH))),
        ('Frame with Chunk',
         proto.Frame(proto.Chunk(transfer_id=tid, item_id=1,
                                 data=pattern(3)))),
    ]
    encode_out = []
    for name, msg in encode:
        raw = msg.encode()
        assert type(msg).decode(raw) == msg
        encode_out.append({
            'name': name,
            'type': type(msg).__name__,
            'value': msg.to_json(),
            'encoded': raw.hex(),
        })

    ping7 = '0807'
    decode = [
        ('unknown varint field is skipped', 'Ping', '0807' + '5005'),
        ('unknown LEN field is skipped', 'Ping', '0807' + '5a03616263'),
        ('unknown I64 field is skipped', 'Ping', '5101020304050607080807'),
        ('unknown I32 field is skipped', 'Ping', '5d010203040807'),
        ('large unknown field number is skipped', 'Ping',
         'f8ffffff0f01' + ping7),
        ('known field with the wrong wire type is skipped', 'Ping',
         '0a0101' + ping7),
        ('last value wins', 'Ping', '0801' + ping7),
        ('fields out of order', 'Done', '1001' + '0a10' + tid.hex()),
        ('explicit default value', 'Ping', '0800'),
        ('non-minimal varint', 'Ping', '088700'),
        ('uint32 keeps the low 32 bits', 'Chunk',
         '1081808080' + '10'),
        ('unpacked repeated uint32', 'Response', '1002' + '1801' + '1803'),
        ('packed repeated uint32 split in two runs', 'Response',
         '1002' + '1a0101' + '1a0203' + '04'),
        ('mixed packed and unpacked', 'Response', '1a0101' + '1802'),
        ('max uint64 offset', 'Chunk', '18ffffffffffffffffff01'),
        ('bits above 2^64 in a 10-byte varint are dropped', 'Ping',
         '08ffffffffffffffff877f'),
        ('Frame with unknown field and a body', 'Frame',
         '3802' + '2a020807'),
        ('Frame body field with unknown wire type is skipped', 'Frame',
         '0805' + '2a020807'),
    ]
    decode_out = []
    for name, type_name, hex_in in decode:
        msg = proto.MESSAGES[type_name].decode(bytes.fromhex(hex_in))
        decode_out.append({
            'name': name,
            'type': type_name,
            'encoded': hex_in,
            'canonical': msg.encode().hex(),
        })

    invalid = [
        ('truncated varint', 'Ping', '0880', False),
        ('truncated key', 'Ping', '80', False),
        ('varint longer than 10 bytes', 'Ping', '08ffffffffffffffffffff01',
         False),
        ('LEN past the end', 'Chunk', '2205616263', False),
        ('truncated I64', 'Ping', '510102', False),
        ('field number 0', 'Ping', '0001', False),
        ('start group wire type', 'Ping', '0b', False),
        ('end group wire type', 'Ping', '0c', False),
        ('wire type 6', 'Ping', '0e', False),
        ('wire type 7', 'Ping', '0f', False),
        ('invalid UTF-8 in a string', 'OfferItem', '1a02c328', False),
        ('UTF-8 encoded surrogate in a string', 'PeerInfo', '1203eda080',
         False),
        ('invalid nested OfferItem', 'Offer', '1203' + '1a01ff', False),
        ('truncated packed uint32', 'Response', '1a0180', False),
        ('Frame with no body', 'Frame', '', True),
        ('Frame with only unknown fields', 'Frame', '3801', True),
        ('Frame with two bodies', 'Frame', '2a00' + '2a020801', True),
        ('Frame with two different bodies', 'Frame', '2a00' + '2200', True),
    ]
    invalid_out = []
    for name, type_name, hex_in, accepted_by_protobuf in invalid:
        try:
            proto.MESSAGES[type_name].decode(bytes.fromhex(hex_in))
        except proto.DecodeError:
            pass
        else:
            raise AssertionError(f'{name} should not decode')
        invalid_out.append({
            'name': name,
            'type': type_name,
            'encoded': hex_in,
            'protobuf_accepts': accepted_by_protobuf,
        })
    return {
        'description':
        'Wire encoding of brave_transfer.proto (PROTOCOL.md section 5). '
        '"encode": value <-> canonical encoding, both directions. "decode": '
        'non-canonical input that must decode, shown with its canonical '
        're-encoding. "invalid": input that must be rejected; '
        '"protobuf_accepts" marks rules stricter than the protobuf runtime. '
        'In "value", bytes are hex and all fields are present.',
        'encode': encode_out,
        'decode': decode_out,
        'invalid': invalid_out,
    }


# ----- filename_sanitizer.json ---------------------------------------------


def filename_sanitizer():
    inputs = [
        'photo.jpg',
        'Résumé 2026.pdf',
        'e\u0301.txt',
        '../../etc/passwd',
        'a/b\\c<d>e:f"g|h?i*j',
        '  .hidden. ',
        'name.',
        '',
        '...',
        ' . ',
        '\u0000\u0007',
        'tab\there.txt',
        'line\nbreak.txt',
        'del\u007f.txt',
        'c1\u0085control.txt',
        'invoice\u202etxt.exe',
        'a\u200bb\u200fc.txt',
        '\ufeffbom.txt',
        'arabic\u061cmark.txt',
        'word\u2060joiner\u2066isolate\u2069.txt',
        'CON',
        'con.txt',
        'Nul.tar.gz',
        'COM1.log',
        'LPT9',
        'COM0.txt',
        'CONSOLE.txt',
        'aux .txt',
        'ﬀ.txt',
        'cᴏn.txt',
        'İ',
        ' PRN.txt',
        'a' * 255,
        'a' * 300 + '.txt',
        'a' * 250 + '.' + 'b' * 40,
        'é' * 200 + '.png',
        '😀' * 70 + '.txt',
        'a' * 249 + '  ..' + 'x' * 10 + '.txt',
        'x.' + 'é' * 20 + '😀' * 2,
        'a' * 300 + '.' + 'é' * 16,
        '😀',
    ]
    return {
        'description':
        'Receiver-side file name sanitization (PROTOCOL.md section 7).',
        'cases': [{
            'input': s,
            'output': sanitize_filename(s)
        } for s in inputs],
    }


# ----- offer_validation.json -----------------------------------------------


def _item(**kw):
    defaults = dict(item_id=1,
                    kind=proto.ITEM_KIND_FILE,
                    name='file.bin',
                    mime_type='application/octet-stream',
                    size=100,
                    sha256=labelled('file'))
    defaults.update(kw)
    return proto.OfferItem(**defaults)


def _inline(kind, data: bytes, **kw):
    return _item(kind=kind,
                 size=len(data),
                 sha256=hashlib.sha256(data).digest(),
                 inline_data=data,
                 name='text.txt',
                 mime_type='text/plain',
                 **kw)


def offer_validation():
    tid = labelled('transfer id', 16)
    url = b'https://brave.com/'
    big_text = b'a' * 9000
    small = b'a' * 8192
    cases = [
        ('one file', [_item()], True),
        ('zero-size file', [_item(size=0, sha256=hashlib.sha256().digest())],
         True),
        ('inline text', [_inline(proto.ITEM_KIND_TEXT, b'hi')], True),
        ('8192-byte inline text', [_inline(proto.ITEM_KIND_TEXT, small)],
         True),
        ('chunked text over 8192 bytes', [
            _item(kind=proto.ITEM_KIND_TEXT,
                  size=len(big_text),
                  sha256=hashlib.sha256(big_text).digest())
        ], True),
        ('url', [_inline(proto.ITEM_KIND_URL, url)], True),
        ('upper-case url scheme', [_inline(proto.ITEM_KIND_URL, b'HTTP://x')],
         True),
        ('256 items', [_item(item_id=i) for i in range(1, 257)], True),
        ('item ids need not be contiguous',
         [_item(item_id=7), _item(item_id=3)], True),
        ('item size 2^40', [_item(size=1 << 40)], True),
        ('name of 1024 bytes', [_item(name='é' * 512)], True),
        ('mime type of 255 bytes', [_item(mime_type='a' * 255)], True),
        ('no items', [], False),
        ('257 items', [_item(item_id=i) for i in range(1, 258)], False),
        ('duplicate item id', [_item(), _item()], False),
        ('unspecified kind', [_item(kind=0)], False),
        ('unknown kind', [_item(kind=5)], False),
        ('empty name', [_item(name='')], False),
        ('name over 1024 bytes', [_item(name='é' * 512 + 'a')], False),
        ('mime type over 255 bytes', [_item(mime_type='a' * 256)], False),
        ('short sha256', [_item(sha256=bytes(31))], False),
        ('item larger than 2^40', [_item(size=(1 << 40) + 1)], False),
        ('total larger than 2^40',
         [_item(size=1 << 39), _item(item_id=2, size=(1 << 39) + 1)], False),
        ('empty text', [
            _item(kind=proto.ITEM_KIND_TEXT,
                  size=0,
                  sha256=hashlib.sha256().digest())
        ], False),
        ('small text not inline', [
            _item(kind=proto.ITEM_KIND_TEXT,
                  size=2,
                  sha256=hashlib.sha256(b'hi').digest())
        ], False),
        ('url not inline', [
            _item(kind=proto.ITEM_KIND_URL,
                  size=len(url),
                  sha256=hashlib.sha256(url).digest())
        ], False),
        ('inline data on a file', [_inline(proto.ITEM_KIND_FILE, b'hi')],
         False),
        ('inline text over 8192 bytes',
         [_inline(proto.ITEM_KIND_TEXT, big_text)], False),
        ('inline length differs from size', [
            _item(kind=proto.ITEM_KIND_TEXT,
                  size=3,
                  sha256=hashlib.sha256(b'hi').digest(),
                  inline_data=b'hi')
        ], False),
        ('inline hash mismatch', [
            _item(kind=proto.ITEM_KIND_TEXT,
                  size=2,
                  sha256=bytes(32),
                  inline_data=b'hi')
        ], False),
        ('inline text not UTF-8', [_inline(proto.ITEM_KIND_TEXT, b'\xff')],
         False),
        ('javascript url', [_inline(proto.ITEM_KIND_URL, b'javascript:1')],
         False),
        ('file url', [_inline(proto.ITEM_KIND_URL, b'file:///etc/passwd')],
         False),
        ('url without scheme', [_inline(proto.ITEM_KIND_URL, b'brave.com')],
         False),
    ]
    out = []
    for name, items, valid in cases:
        offer = proto.Offer(transfer_id=tid, items=items)
        out.append(_validation_case(name, offer, valid))
    for name, transfer_id in (('short transfer id', tid[:15]),
                              ('long transfer id', tid + b'\0'),
                              ('missing transfer id', b'')):
        out.append(
            _validation_case(name,
                             proto.Offer(transfer_id=transfer_id,
                                         items=[_item()]), False))
    return {
        'description':
        'Offer validation with default limits (PROTOCOL.md section 8.2). '
        'Receivers reject an invalid offer by closing the connection. When '
        '"item_count" is present, the offer\'s items are "item_template" '
        'repeated with item_id 1..item_count.',
        'cases': out,
    }


def _validation_case(name, offer, valid):
    try:
        validate_offer(offer, Limits())
        error = None
    except ProtocolError as e:
        error = str(e)
    if (error is None) != valid:
        raise AssertionError(f'{name}: expected valid={valid}, got {error}')
    case = {'name': name, 'valid': valid}
    template = offer.items[0] if len(offer.items) > 2 else None
    if template and all(
            item == proto.OfferItem(**{**vars(template), 'item_id': i})
            for i, item in enumerate(offer.items, start=1)):
        case['offer'] = {'transfer_id': offer.transfer_id.hex(), 'items': []}
        case['item_template'] = template.to_json()
        case['item_count'] = len(offer.items)
    else:
        case['offer'] = offer.to_json()
    if error:
        case['reference_error'] = error
    return case


# ----- session_transcript.json ---------------------------------------------


def scenario_items():
    text = 'Hello from Brave Transfer 👋'.encode()
    notes = b'0123456789abcdef' * 563  # 9008 bytes: TEXT that is chunked
    return [
        ('text', proto.ITEM_KIND_TEXT, 'text.txt', 'text/plain', text),
        ('url', proto.ITEM_KIND_URL, 'brave.com', 'text/uri-list',
         b'https://brave.com/transfer'),
        ('pattern', proto.ITEM_KIND_IMAGE, 'photo.jpg', 'image/jpeg',
         pattern(62440)),
        ('empty', proto.ITEM_KIND_FILE, 'empty.txt', 'text/plain', b''),
        ('text', proto.ITEM_KIND_TEXT, 'notes.txt', 'text/plain', notes),
    ]


SCENARIOS = [
    ('accept_all', proto.DECISION_ACCEPT_ALL, []),
    ('accept_some', proto.DECISION_ACCEPT_SOME, [2, 3]),
    ('accept_some_inline_only', proto.DECISION_ACCEPT_SOME, [1, 2, 4]),
    ('decline', proto.DECISION_DECLINE, []),
]


def run_scenario(decision, accepted_ids, items, transfer_id):
    """Runs one transfer between two in-memory sessions.

    Delivery order, which other implementations must reproduce to get the
    same transcript: repeatedly drain the responder's frames and then the
    initiator's, delivering each frame to the other side as soon as it is
    produced, until neither side has anything to send. The responder answers
    an offer as soon as it receives it.
    """
    now = SESSION_START
    initiator = TransferSession(now, random=lambda n: transfer_id[:n])
    responder = TransferSession(now, send_confirmation=True)
    initiator.send_offer(items, now)
    transcript, finished = [], {}

    def deliver(receiver, name, frame):
        events = receiver.handle_frame(proto.Frame.decode(frame.encode()), now)
        while events:
            event = events.pop(0)
            if isinstance(event, OfferReceived):
                events += receiver.respond(event.offer.transfer_id, decision,
                                           accepted_ids)
            elif isinstance(event, TransferFinished):
                finished[name] = event

    progressed = True
    while progressed:
        progressed = False
        for sender, receiver, name, other in ((responder, initiator,
                                               'responder', 'initiator'),
                                              (initiator, responder,
                                               'initiator', 'responder')):
            while (frame := sender.next_frame(now)) is not None:
                transcript.append((name, frame))
                deliver(receiver, other, frame)
                progressed = True
    assert initiator.idle and responder.idle
    return transcript, finished


def session_transcript():
    tid = labelled('transfer id', 16)
    items_spec = scenario_items()
    items = [
        OutgoingItem.from_bytes(kind, name, mime, data)
        for _, kind, name, mime, data in items_spec
    ]
    scenarios = []
    for name, decision, accepted in SCENARIOS:
        transcript, finished = run_scenario(decision, accepted, items, tid)
        frames = []
        for sender, frame in transcript:
            raw = frame.encode()
            entry = {
                'sender': sender,
                'type': next(iter(frame.to_json())),
                'length': len(raw),
                'sha256': hashlib.sha256(raw).hexdigest(),
            }
            if len(raw) <= 1024:
                entry['plaintext'] = raw.hex()
            frames.append(entry)
        scenarios.append({
            'name': name,
            'decision': decision,
            'accepted_item_ids': accepted,
            'frames': frames,
            'initiator_result': _finished_json(finished['initiator']),
            'responder_result': _finished_json(finished['responder']),
        })

    def item_json(spec):
        source, kind, name, mime, data = spec
        out = {'kind': kind, 'name': name, 'mime_type': mime}
        if source == 'pattern':
            out['pattern_length'] = len(data)
        else:
            out['text'] = data.decode('utf-8')
        return out

    return {
        'description':
        'Frame sequences for complete transfers between two TransferSessions '
        '(PROTOCOL.md section 8). The initiator offers "items" with '
        'transfer_id; the responder answers with each scenario\'s decision. '
        'Item contents are UTF-8 "text" or "pattern_length" bytes where byte '
        'i is i % 251. Delivery order is described in run_scenario() in '
        'tools/brave_transfer/gen_vectors.py. Frames over 1024 bytes are '
        'given by length and SHA-256 only.',
        'transfer_id': tid.hex(),
        'items': [item_json(s) for s in items_spec],
        'scenarios': scenarios,
    }


def _finished_json(event):
    return {
        'status': event.status,
        'cancel_reason': event.cancel_reason,
        'decision': event.decision,
    }


# ----- main ----------------------------------------------------------------


def build(cacophony_source: Path):
    files = {
        'key_derivation.json': key_derivation(),
        'discovery_tags.json': discovery_tags(),
        'noise_handshake.json': noise_handshake(),
        'proto_messages.json': proto_messages(),
        'filename_sanitizer.json': filename_sanitizer(),
        'offer_validation.json': offer_validation(),
        'session_transcript.json': session_transcript(),
    }
    out = {
        name: json.dumps(value, indent=2, ensure_ascii=False) + '\n'
        for name, value in files.items()
    }
    if cacophony_source:
        out['noise_cacophony_xxpsk3.json'] = json.dumps(
            cacophony(cacophony_source), indent=2) + '\n'
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=DATA_DIR)
    parser.add_argument('--check',
                        action='store_true',
                        help='fail if the files in --out are out of date')
    parser.add_argument(
        '--cacophony',
        type=Path,
        help='cacophony.txt from ' + CACOPHONY_SOURCE +
        ' to (re)write noise_cacophony_xxpsk3.json')
    args = parser.parse_args()
    stale = []
    for name, text in build(args.cacophony).items():
        path = args.out / name
        if args.check:
            if not path.exists() or path.read_text() != text:
                stale.append(name)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
    if stale:
        sys.exit('out of date: ' + ', '.join(stale) +
                 '; run tools/brave_transfer/gen_vectors.py')


if __name__ == '__main__':
    main()
