# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import hashlib

import pytest

import gen_vectors
from brave_transfer_ref import proto
from brave_transfer_ref.channel import ProtocolError
from brave_transfer_ref.session import (IDLE_TIMEOUT, OFFER_TIMEOUT,
                                        PING_INTERVAL, ChunkReceived,
                                        ItemReceived, OfferReceived,
                                        OutgoingItem, ResponseReceived,
                                        TransferFinished, TransferSession)


def _items():
    return [
        OutgoingItem.from_bytes(kind, name, mime, data)
        for _, kind, name, mime, data in gen_vectors.scenario_items()
    ]


def _drain(session, now=0.0):
    frames = []
    while (f := session.next_frame(now)) is not None:
        frames.append(f)
    return frames


def _deliver(frames, receiver, now=0.0):
    events = []
    for f in frames:
        events += receiver.handle_frame(proto.Frame.decode(f.encode()), now)
    return events


def _finished(events):
    return [e for e in events if isinstance(e, TransferFinished)]


def test_transcripts_match_vectors(vectors):
    data = vectors('session_transcript.json')
    tid = bytes.fromhex(data['transfer_id'])
    for scenario in data['scenarios']:
        transcript, finished = gen_vectors.run_scenario(
            scenario['decision'], scenario['accepted_item_ids'], _items(),
            tid)
        assert [(s, hashlib.sha256(f.encode()).hexdigest())
                for s, f in transcript] == [(f['sender'], f['sha256'])
                                            for f in scenario['frames']]
        assert finished['initiator'].status == \
            scenario['initiator_result']['status']


def test_received_chunks_reassemble():
    sender, receiver = TransferSession(0), TransferSession(0)
    items = _items()
    sender.send_offer(items, 0)
    events = _deliver(_drain(sender), receiver)
    (offer, ) = [e for e in events if isinstance(e, OfferReceived)]
    receiver.respond(offer.offer.transfer_id, proto.DECISION_ACCEPT_ALL)
    _deliver(_drain(receiver), sender)
    events = _deliver(_drain(sender), receiver)
    data = {}
    for e in events:
        if isinstance(e, ChunkReceived):
            data[e.item_id] = data.get(e.item_id, b'') + e.data
    assert data[3] == gen_vectors.pattern(62440)
    assert {e.item.item_id
            for e in events if isinstance(e, ItemReceived)} == {3, 5}
    assert _finished(_deliver(_drain(receiver), sender))[0].status == \
        proto.DONE_STATUS_OK


def _offer(sender, receiver, items=None):
    tid = sender.send_offer(items or _items(), 0)
    _deliver(_drain(sender), receiver)
    return tid


def test_second_incoming_offer_is_busy():
    a, b, c = TransferSession(0), TransferSession(0), TransferSession(0)
    _offer(a, b)
    tid = c.send_offer(_items(), 0)
    events = _deliver(_drain(c), b)
    assert not events
    (resp, ) = _drain(b)
    assert resp.body == proto.Response(transfer_id=tid,
                                       decision=proto.DECISION_BUSY)
    events = _deliver([resp], c)
    assert _finished(events)[0].decision == proto.DECISION_BUSY


def test_both_directions_at_once():
    a, b = TransferSession(0), TransferSession(0)
    ta = a.send_offer(_items()[:1], 0)
    tb = b.send_offer(_items()[:1], 0)
    _deliver(_drain(a), b)
    _deliver(_drain(b), a)
    # Inline-only acceptances finish as soon as the receiver responds.
    ea = a.respond(tb, proto.DECISION_ACCEPT_ALL)
    eb = b.respond(ta, proto.DECISION_ACCEPT_ALL)
    ea += _deliver(_drain(b), a)
    eb += _deliver(_drain(a), b)
    assert a.idle and b.idle
    assert sorted((e.outgoing, e.status) for e in _finished(ea + eb)) == [
        (False, proto.DONE_STATUS_OK)] * 2 + [(True, proto.DONE_STATUS_OK)] * 2


def test_hash_mismatch():
    data = b'x' * 100000
    bad = OutgoingItem(proto.ITEM_KIND_FILE, 'f', '', len(data),
                       hashlib.sha256(b'other').digest(), b'',
                       lambda o, n: data[o:o + n])
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r, [bad])
    r.respond(tid, proto.DECISION_ACCEPT_ALL)
    _deliver(_drain(r), s)
    events = _deliver(_drain(s), r)
    assert _finished(events)[0].status == proto.DONE_STATUS_HASH_MISMATCH
    events = _deliver(_drain(r), s)
    assert _finished(events)[0].status == proto.DONE_STATUS_HASH_MISMATCH


def test_cancel_by_sender_mid_transfer():
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    r.respond(tid, proto.DECISION_ACCEPT_ALL)
    _deliver(_drain(r), s)
    first_chunk = s.next_frame(0)
    s.cancel(tid)
    rest = _drain(s)
    assert [type(f.body) for f in rest] == [proto.Cancel]
    events = _deliver([first_chunk] + rest, r)
    assert _finished(events)[0].cancel_reason == proto.CANCEL_REASON_USER
    assert r.idle and s.idle
    # Frames still in flight for the cancelled transfer are ignored.
    assert _deliver([first_chunk], r) == []


def test_write_failure():
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    r.respond(tid, proto.DECISION_ACCEPT_ALL)
    _deliver(_drain(r), s)
    _deliver([s.next_frame(0)], r)
    assert _finished(r.write_failed(tid))[0].status == \
        proto.DONE_STATUS_WRITE_FAILED
    events = _deliver(_drain(r), s)
    assert _finished(events)[0].status == proto.DONE_STATUS_WRITE_FAILED
    assert s.idle and r.idle


def test_cancel_by_receiver_while_pending():
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    r.cancel(tid)
    events = _deliver(_drain(r), s)
    assert _finished(events)[0].outgoing


def test_offer_timeouts():
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    t = OFFER_TIMEOUT + 1
    s.last_received = r.last_received = t
    events = s.poll(t)
    assert _finished(events)[0].cancel_reason == proto.CANCEL_REASON_TIMEOUT
    assert _drain(s, t)[0].body == proto.Cancel(
        transfer_id=tid, reason=proto.CANCEL_REASON_TIMEOUT)
    events = r.poll(t)
    assert _finished(events)[0].decision == proto.DECISION_DECLINE


def test_pings_and_idle_timeout():
    s = TransferSession(0)
    assert s.poll(PING_INTERVAL - 1) == [] and s.next_frame(0) is None
    s.poll(PING_INTERVAL)
    assert s.next_frame(PING_INTERVAL).body == proto.Ping(sequence=1)
    with pytest.raises(ProtocolError, match='idle'):
        s.poll(IDLE_TIMEOUT + 0.5)


def test_confirmation_ping_comes_first():
    s = TransferSession(0, send_confirmation=True)
    s.send_offer(_items()[:1], 0)
    assert s.next_frame(0).body == proto.Ping(sequence=0)


@pytest.mark.parametrize('mutate,error', [
    (lambda c: setattr(c, 'offset', c.offset + 1), 'out-of-order'),
    (lambda c: setattr(c, 'item_id', 99), 'out-of-order'),
    (lambda c: setattr(c, 'data', b''), 'bad chunk length'),
    (lambda c: setattr(c, 'transfer_id', bytes(16)), 'unknown transfer'),
])
def test_bad_chunks_are_protocol_errors(mutate, error):
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    r.respond(tid, proto.DECISION_ACCEPT_ALL)
    _deliver(_drain(r), s)
    chunk = s.next_frame(0)
    mutate(chunk.body)
    with pytest.raises(ProtocolError, match=error):
        _deliver([chunk], r)


def test_chunk_past_item_end_is_rejected():
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    r.respond(tid, proto.DECISION_ACCEPT_ALL)
    _deliver(_drain(r), s)
    chunk = s.next_frame(0)
    chunk.body.data += s.next_frame(0).body.data[:1]
    with pytest.raises(ProtocolError):
        _deliver([chunk], r)


@pytest.mark.parametrize('response,error', [
    (dict(decision=proto.DECISION_ACCEPT_ALL, accepted_item_ids=[1]),
     'ACCEPT_ALL'),
    (dict(decision=proto.DECISION_ACCEPT_SOME), 'accepted'),
    (dict(decision=proto.DECISION_ACCEPT_SOME, accepted_item_ids=[1, 1]),
     'accepted'),
    (dict(decision=proto.DECISION_ACCEPT_SOME, accepted_item_ids=[42]),
     'accepted'),
    (dict(decision=proto.DECISION_DECLINE, accepted_item_ids=[1]),
     'refusal'),
    (dict(decision=0), 'unknown decision'),
])
def test_bad_responses_are_protocol_errors(response, error):
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    with pytest.raises(ProtocolError, match=error):
        s.handle_frame(proto.Frame(proto.Response(transfer_id=tid,
                                                  **response)), 0)


def test_done_ok_before_all_chunks_is_an_error():
    s, r = TransferSession(0), TransferSession(0)
    tid = _offer(s, r)
    r.respond(tid, proto.DECISION_ACCEPT_ALL)
    events = _deliver(_drain(r)[:1], s)
    assert isinstance(events[0], ResponseReceived)
    with pytest.raises(ProtocolError):
        s.handle_frame(
            proto.Frame(proto.Done(transfer_id=tid,
                                   status=proto.DONE_STATUS_OK)), 0)


def test_reused_transfer_id_is_an_error():
    a, b = TransferSession(0, random=lambda n: b'\1' * n), TransferSession(0)
    tid = _offer(a, b)
    b.respond(tid, proto.DECISION_DECLINE)
    _deliver(_drain(b), a)
    a.send_offer(_items(), 0)
    with pytest.raises(ProtocolError, match='reused'):
        _deliver(_drain(a), b)
