# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import hashlib

import pytest

import gen_vectors
from brave_transfer_ref import bip39, proto
from brave_transfer_ref.keys import ChainKeys, epoch_for, fingerprint
from brave_transfer_ref.sanitize import sanitize_filename


def test_committed_vectors_are_up_to_date():
    for name, text in gen_vectors.build(None).items():
        path = gen_vectors.DATA_DIR / name
        assert path.read_text() == text, (
            f'{name} is stale; run tools/brave_transfer/gen_vectors.py')


def test_wordlist_is_the_bip39_english_list():
    text = '\n'.join(bip39.WORDS) + '\n'
    assert hashlib.sha256(text.encode()).hexdigest() == (
        '2f5eed53a4727b4bf8880d8f3f199efc90e58503646d9ff8eff3a2ed3b24dbda')


def test_well_known_bip39_mnemonics(vectors):
    valid = {v['name']: v for v in vectors('key_derivation.json')['valid']}
    assert valid['all zero entropy']['sync_code'] == ' '.join(['abandon'] *
                                                              23 + ['art'])
    assert valid['all 0xff entropy']['sync_code'] == ' '.join(['zoo'] * 23 +
                                                              ['vote'])


def test_key_derivation(vectors):
    data = vectors('key_derivation.json')
    for v in data['valid']:
        assert bip39.mnemonic_to_bytes32(v['sync_code']).hex() == v['seed']
        assert bip39.bytes32_to_mnemonic(bytes.fromhex(
            v['seed'])) == v['sync_code']
        keys = ChainKeys.from_sync_code(v['sync_code'])
        assert keys.root.hex() == v['root_key']
        assert keys.psk.hex() == v['psk']
        assert keys.discovery.hex() == v['discovery_key']
    for v in data['invalid_sync_codes']:
        with pytest.raises(bip39.InvalidMnemonic):
            bip39.mnemonic_to_bytes32(v['sync_code'])


def test_discovery_tags(vectors):
    data = vectors('discovery_tags.json')
    keys = ChainKeys.from_seed(bytes.fromhex(data['seed']))
    assert keys.discovery.hex() == data['discovery_key']
    for v in data['tags']:
        assert epoch_for(v['unix_time']) == v['epoch']
        assert keys.tag(v['epoch']).hex() == v['tag']
    for v in data['accepted']:
        assert [t.hex() for t in keys.accepted_tags(v['unix_time'])
                ] == v['tags']


def test_fingerprints(vectors):
    data = vectors('noise_handshake.json')
    for side in ('initiator', 'responder'):
        assert fingerprint(bytes.fromhex(
            data[f'{side}_static_public'])) == data[f'{side}_fingerprint']


def test_filename_sanitizer(vectors):
    for case in vectors('filename_sanitizer.json')['cases']:
        out = sanitize_filename(case['input'])
        assert out == case['output']
        assert 1 <= len(out.encode()) <= 255
        assert sanitize_filename(out) == out


def test_proto_vectors_round_trip(vectors):
    data = vectors('proto_messages.json')
    for case in data['encode']:
        cls = proto.MESSAGES[case['type']]
        msg = cls.from_json(case['value'])
        assert msg.encode().hex() == case['encoded'], case['name']
        assert cls.decode(bytes.fromhex(case['encoded'])) == msg
    for case in data['decode']:
        cls = proto.MESSAGES[case['type']]
        msg = cls.decode(bytes.fromhex(case['encoded']))
        assert msg.encode().hex() == case['canonical'], case['name']
    for case in data['invalid']:
        with pytest.raises(proto.DecodeError):
            proto.MESSAGES[case['type']].decode(bytes.fromhex(
                case['encoded']))
