# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

import pytest

from brave_transfer_ref import noise, proto
from brave_transfer_ref.channel import (InitiatorHandshake, ProtocolError,
                                        ResponderHandshake)
from brave_transfer_ref.keys import ChainKeys
from brave_transfer_ref.noise import HandshakeState, NoiseError


def test_cacophony_vector(vectors):
    v = vectors('noise_cacophony_xxpsk3.json')
    h = lambda k: bytes.fromhex(v[k])
    init = HandshakeState(True, h('init_prologue'), h('init_static'),
                          bytes.fromhex(v['init_psks'][0]),
                          h('init_ephemeral'))
    resp = HandshakeState(False, h('resp_prologue'), h('resp_static'),
                          bytes.fromhex(v['resp_psks'][0]),
                          h('resp_ephemeral'))
    for i, m in enumerate(v['messages']):
        payload, ciphertext = bytes.fromhex(m['payload']), bytes.fromhex(
            m['ciphertext'])
        sender, receiver = (init, resp) if i % 2 == 0 else (resp, init)
        if i < 3:
            assert sender.write_message(payload) == ciphertext
            assert receiver.read_message(ciphertext) == payload
        else:
            assert sender.send_cipher.encrypt(b'', payload) == ciphertext
            assert receiver.recv_cipher.decrypt(b'', ciphertext) == payload
        if i == 2:
            assert init.handshake_hash.hex() == v['handshake_hash']
            assert resp.handshake_hash == init.handshake_hash


def _run_brave_vector(v):
    b = lambda k: bytes.fromhex(v[k])
    tag = b('trust_tag')
    infos = [
        proto.PeerInfo.from_json(m['payload_message'])
        for m in v['handshake_messages']
    ]
    init = InitiatorHandshake(b('initiator_static'), b('psk'), tag, infos[2],
                              b('initiator_ephemeral'))
    resp = ResponderHandshake(b('responder_static'), lambda t: b('psk')
                              if t == tag else None, infos[1],
                              b('responder_ephemeral'))
    msgs = [bytes.fromhex(m['message']) for m in v['handshake_messages']]
    assert init.message1() == msgs[0]
    assert resp.read_message1(msgs[0]).trust_tag == tag
    assert resp.message2() == msgs[1]
    assert init.read_message2(msgs[1]) == infos[1]
    assert init.message3() == msgs[2]
    assert resp.read_message3(msgs[2]) == infos[2]
    return init.channel(), resp.channel()


def test_brave_handshake_vector(vectors):
    v = vectors('noise_handshake.json')
    ic, rc = _run_brave_vector(v)
    assert ic.handshake_hash.hex() == v['handshake_hash']
    assert ic.send_cipher.key.hex() == v['initiator_to_responder_key']
    assert rc.send_cipher.key.hex() == v['responder_to_initiator_key']
    assert ic.remote_static.hex() == v['responder_static_public']
    assert rc.remote_static.hex() == v['initiator_static_public']
    assert not ic.confirmed
    for m in v['transport_messages']:
        sender, receiver = (ic, rc) if m['sender'] == 'initiator' else (rc,
                                                                         ic)
        frame = proto.Frame.from_json(m['frame'])
        assert frame.encode().hex() == m['plaintext']
        wire = sender.seal(frame)
        assert wire.hex() == m['wire']
        assert receiver.open(wire[2:]) == frame
        assert ic.confirmed


def _handshake(i_keys, r_keys, tag=None):
    info = proto.PeerInfo(protocol_version=1, device_name='x')
    tag = tag or i_keys.tag(1000)
    init = InitiatorHandshake(os.urandom(32), i_keys.psk, tag, info)
    resp = ResponderHandshake(
        os.urandom(32), lambda t: r_keys.psk
        if t in (r_keys.tag(999), r_keys.tag(1000), r_keys.tag(1001)) else
        None, info)
    resp.read_message1(init.message1())
    init.read_message2(resp.message2())
    resp.read_message3(init.message3())
    return init.channel(), resp.channel()


def test_different_chain_is_rejected_by_tag():
    with pytest.raises(ProtocolError, match='unknown trust tag'):
        _handshake(ChainKeys.from_seed(bytes(32)),
                   ChainKeys.from_seed(b'\1' * 32))


def test_wrong_psk_fails_at_message_3():
    a, b = ChainKeys.from_seed(bytes(32)), ChainKeys.from_seed(b'\1' * 32)
    info = proto.PeerInfo(protocol_version=1)
    init = InitiatorHandshake(os.urandom(32), a.psk, b.tag(1000), info)
    resp = ResponderHandshake(os.urandom(32), lambda t: b.psk, info)
    resp.read_message1(init.message1())
    init.read_message2(resp.message2())
    with pytest.raises(ProtocolError, match='message 3'):
        resp.read_message3(init.message3())


def test_responder_without_psk_cannot_confirm():
    # An attacker that saw the tag can answer messages 1 and 2, but any
    # transport message it sends fails to decrypt on the initiator.
    keys = ChainKeys.from_seed(bytes(32))
    info = proto.PeerInfo(protocol_version=1)
    init = InitiatorHandshake(os.urandom(32), keys.psk, keys.tag(5), info)
    fake = ResponderHandshake(os.urandom(32), lambda t: os.urandom(32), info)
    fake.read_message1(init.message1())
    init.read_message2(fake.message2())
    m3 = init.message3()
    with pytest.raises(ProtocolError):
        fake.read_message3(m3)
    ic = init.channel()
    with pytest.raises(ProtocolError):
        ic.open(os.urandom(40))
    assert not ic.confirmed


def test_tags_from_adjacent_epochs_are_accepted():
    keys = ChainKeys.from_seed(bytes(32))
    for epoch in (999, 1000, 1001):
        _handshake(keys, keys, keys.tag(epoch))
    with pytest.raises(ProtocolError):
        _handshake(keys, keys, keys.tag(1002))


def test_tampered_transport_message_is_rejected():
    keys = ChainKeys.from_seed(bytes(32))
    ic, rc = _handshake(keys, keys)
    wire = bytearray(ic.seal(proto.Frame(proto.Ping(sequence=1))))
    wire[-1] ^= 1
    with pytest.raises(ProtocolError):
        rc.open(bytes(wire[2:]))


def test_low_order_public_keys_are_rejected():
    # 0 and 1 are low-order points on Curve25519.
    for point in (bytes(32), b'\1' + bytes(31)):
        with pytest.raises(NoiseError, match='low-order'):
            noise._dh(os.urandom(32), point)


def test_out_of_turn_messages_fail():
    hs = HandshakeState(False, b'', os.urandom(32))
    with pytest.raises(NoiseError):
        hs.write_message(b'')


def test_oversized_control_frame_is_rejected():
    keys = ChainKeys.from_seed(bytes(32))
    ic, _ = _handshake(keys, keys)
    offer = proto.Offer(transfer_id=bytes(16),
                        items=[proto.OfferItem(name='a' * 17000)])
    with pytest.raises(ProtocolError, match='too large'):
        ic.seal(proto.Frame(offer))
