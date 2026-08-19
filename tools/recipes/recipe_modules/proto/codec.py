# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The three protobuf wire formats the `proto` module can read and write.

Each codec bundles the file extension it uses, the encode/decode functions, and
the keyword defaults that make its output stable enough to put in an
expectation. Kept apart from `api.py` so the module's `test_api.py` can reach
them without importing the api.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Callable

from google.protobuf import json_format, text_format
from google.protobuf.message import Message


@dataclass(frozen=True)
class Codec:
    """One protobuf wire format."""

    #: The codec's name, as recipes spell it (e.g. `'JSONPB'`).
    name: str
    #: Extension for the temp files placeholders of this codec render to.
    ext: str
    #: `(message, **kwargs) -> str | bytes`.
    encode: Callable[..., Any]
    #: `(data, message, **kwargs) -> None`, parsing *data* into *message*.
    decode: Callable[..., Any]
    #: Keyword defaults for `encode`, overridable per call.
    encode_defaults: dict[str, Any] = field(default_factory=dict)
    #: Keyword defaults for `decode`, overridable per call.
    decode_defaults: dict[str, Any] = field(default_factory=dict)

    @property
    def is_binary(self) -> bool:
        """Whether this codec's encoded form is bytes rather than text."""
        return self is BINARY


BINARY = Codec(
    name='BINARY',
    ext='pb',
    encode=lambda msg, **extra: msg.SerializeToString(**extra),
    decode=lambda data, msg, **_extra: msg.ParseFromString(data),
    # Deterministic output, so encoding the same message twice (in two builds,
    # say) gives byte-identical results.
    encode_defaults={'deterministic': True},
)

TEXTPB = Codec(
    name='TEXTPB',
    ext='tpb',
    encode=text_format.MessageToString,
    decode=text_format.Parse,
)

JSONPB = Codec(
    name='JSONPB',
    ext='json',
    encode=json_format.MessageToJson,
    decode=json_format.Parse,
    encode_defaults={
        'preserving_proto_field_name': True,
        'sort_keys': True,
        'indent': 2,
    },
    # A message written by an older or newer build of the same schema still
    # parses; unknown fields are dropped rather than fatal.
    decode_defaults={'ignore_unknown_fields': True},
)

ALL_CODECS = (BINARY, JSONPB, TEXTPB)
_BY_NAME = {codec.name: codec for codec in ALL_CODECS}


def resolve(codec: Codec | str) -> Codec:
    """Return the `Codec` for *codec*, given either a codec or its name."""
    if isinstance(codec, str):
        codec = _BY_NAME.get(codec, codec)
    if codec not in ALL_CODECS:
        raise ValueError(f'Must specify a valid codec, got {codec!r}; '
                         f'expected one of {sorted(_BY_NAME)}')
    return codec


def do_encode(proto_msg: Message, codec: Codec | str, **extra) -> Any:
    """Encode *proto_msg* with *codec*."""
    codec = resolve(codec)
    return codec.encode(proto_msg, **{**codec.encode_defaults, **extra})


def do_decode(data: Any, msg_class: type[Message], codec: Codec | str,
              **extra) -> Message:
    """Decode *data* into a fresh *msg_class* with *codec*."""
    codec = resolve(codec)
    msg = msg_class()
    codec.decode(data, msg, **{**codec.decode_defaults, **extra})
    return msg
