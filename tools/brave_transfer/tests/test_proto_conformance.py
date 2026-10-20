# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Checks the hand-written codec against protoc + the protobuf runtime."""

import importlib
import sys

import pytest

import gen_vectors

protoc = pytest.importorskip('grpc_tools.protoc')
pytest.importorskip('google.protobuf')

PROTO = gen_vectors.DATA_DIR.parents[1] / 'brave_transfer.proto'


@pytest.fixture(scope='module')
def pb(tmp_path_factory):
    out = tmp_path_factory.mktemp('pb')
    assert protoc.main([
        'protoc', f'-I{PROTO.parent}', f'--python_out={out}',
        str(PROTO)
    ]) == 0
    sys.path.insert(0, str(out))
    try:
        return importlib.import_module('brave_transfer_pb2')
    finally:
        sys.path.remove(str(out))


def _parse(pb, type_name, data):
    msg = getattr(pb, type_name)()
    msg.ParseFromString(data)
    return msg


def test_canonical_encoding_matches_protobuf(pb, vectors):
    for case in vectors('proto_messages.json')['encode']:
        raw = bytes.fromhex(case['encoded'])
        msg = _parse(pb, case['type'], raw)
        assert msg.SerializeToString(deterministic=True) == raw, case['name']


def test_decoding_matches_protobuf(pb, vectors):
    for case in vectors('proto_messages.json')['decode']:
        msg = _parse(pb, case['type'], bytes.fromhex(case['encoded']))
        msg.DiscardUnknownFields()
        assert msg.SerializeToString(deterministic=True).hex() == \
            case['canonical'], case['name']


def test_invalid_inputs_match_protobuf(pb, vectors):
    from google.protobuf.message import DecodeError
    for case in vectors('proto_messages.json')['invalid']:
        data = bytes.fromhex(case['encoded'])
        if case['protobuf_accepts']:
            _parse(pb, case['type'], data)
        else:
            with pytest.raises(DecodeError):
                _parse(pb, case['type'], data)
                pytest.fail(case['name'])


def test_enum_values_match_proto(pb):
    from brave_transfer_ref import proto
    for name in dir(proto):
        if name.split('_')[0] in ('DEVICE', 'ITEM', 'DECISION', 'CANCEL',
                                  'DONE') and name.isupper():
            assert getattr(pb, name) == getattr(proto, name), name
