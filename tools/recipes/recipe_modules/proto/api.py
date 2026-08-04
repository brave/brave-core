# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `proto` module API: protobuf messages in and out of steps.

The same shape as the `json` module, one layer up: `api.proto.output(MsgClass,
'JSONPB')` hands the step a path to write and gives the recipe back a parsed
message, and `api.proto.input(msg, 'JSONPB')` goes the other way. Three wire
formats are available -- `BINARY`, `JSONPB` and `TEXTPB` (see `codec.py`).

Deliberately not supported: `add_json_log` (mirroring the parsed message into a
step log), which needs step presentation.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from google.protobuf.message import Message

from recipe_api import OutputPlaceholder, RecipeApi, returns_placeholder
from recipe_test_api import PlaceholderTestData

from . import codec as codecs


class ProtoOutputPlaceholder(OutputPlaceholder):
    """Renders to a path the step writes a message to, then parses it back.

    A `raw_io` output placeholder with a codec on the end: text-shaped for
    `JSONPB`/`TEXTPB`, bytes-shaped for `BINARY`. A file the step never wrote,
    or wrote something unparseable to, results in `None`.
    """

    def __init__(self, api, msg_class: type[Message], codec: codecs.Codec,
                 name: str | None, leak_to: str | Path | None,
                 decoding_kwargs: dict) -> None:
        suffix = f'.{codec.ext}'
        self.raw = (api.m.raw_io.output(suffix, leak_to=leak_to)
                    if codec.is_binary else api.m.raw_io.output_text(
                        suffix, leak_to=leak_to))
        self._msg_class = msg_class
        self._codec = codec
        self._decoding_kwargs = decoding_kwargs
        super().__init__(name=name)

    @property
    def backing_file(self) -> str | None:
        return self.raw.backing_file

    def render(self, test) -> list[str]:
        return self.raw.render(test)

    def result(self, test) -> Message | None:
        if test.enabled and isinstance(test.data, Message):
            # A test seeds a message, not bytes: the codec is known only here,
            # so encode it now rather than making every test spell it out.
            test = PlaceholderTestData(data=codecs.do_encode(
                test.data, self._codec),
                                       name=self.name)
        raw_data = self.raw.result(test)
        if raw_data is None:
            return None
        try:
            return codecs.do_decode(raw_data, self._msg_class, self._codec,
                                    **self._decoding_kwargs)
        except Exception:  # pylint: disable=broad-except
            # Any of the three parsers may reject the data in its own way; to a
            # recipe they all mean the same thing.
            return None


class ProtoApi(RecipeApi):
    """Encode and decode protobuf messages, and carry them through steps."""

    BINARY = codecs.BINARY.name
    JSONPB = codecs.JSONPB.name
    TEXTPB = codecs.TEXTPB.name

    @returns_placeholder
    def input(self, proto_msg: Message, codec: codecs.Codec | str, **kwargs):
        """A placeholder expanding to the path of a file holding *proto_msg*.

        Args:
            proto_msg: The message to encode.
            codec: Which wire format to encode it in.
            kwargs: Passed to the codec's encoder, overriding its defaults:
                `BINARY` takes `Message.SerializeToString`'s arguments, `JSONPB`
                `json_format.MessageToJson`'s, and `TEXTPB`
                `text_format.MessageToString`'s.
        """
        codec = codecs.resolve(codec)
        encoded = self.encode(proto_msg, codec, **kwargs)
        suffix = f'.{codec.ext}'
        if codec.is_binary:
            return self.m.raw_io.input(encoded, suffix=suffix)
        return self.m.raw_io.input_text(encoded, suffix=suffix)

    @returns_placeholder
    def output(self,
               msg_class: type[Message],
               codec: codecs.Codec | str,
               name: str | None = None,
               leak_to: str | Path | None = None,
               **kwargs) -> ProtoOutputPlaceholder:
        """A placeholder expanding to a path the step writes a message to.

        Once the step is done, the engine parses that file and files the message
        onto the step's result as `step_result.proto.output`.

        Args:
            msg_class: The message type to decode.
            codec: Which wire format the step writes.
            name: Distinguishes this placeholder from others produced by the
                same method on one step. With a name, the result is also at
                `step_result.proto.outputs[name]`.
            leak_to: Write to this path instead of a temporary file, and do not
                delete it afterwards (i.e. "leak" it), so a later step can pick
                it up.
            kwargs: Passed to the codec's decoder, overriding its defaults:
                `BINARY` takes `Message.ParseFromString`'s arguments, `JSONPB`
                `json_format.Parse`'s, and `TEXTPB` `text_format.Parse`'s.
        """
        codec = codecs.resolve(codec)
        if not (isinstance(msg_class, type)
                and issubclass(msg_class, Message)):
            raise ValueError('msg_class must be a protobuf message class; got '
                             f'{msg_class!r}')
        return ProtoOutputPlaceholder(self, msg_class, codec, name, leak_to,
                                      kwargs)

    @staticmethod
    def encode(proto_msg: Message, codec: codecs.Codec | str, **kwargs) -> Any:
        """Encode *proto_msg* with *codec*, returning text or (BINARY) bytes."""
        if not isinstance(proto_msg, Message):
            raise ValueError('proto_msg must be a protobuf message; got '
                             f'{type(proto_msg)}')
        return codecs.do_encode(proto_msg, codec, **kwargs)

    @staticmethod
    def decode(data: Any, msg_class: type[Message], codec: codecs.Codec | str,
               **kwargs) -> Message:
        """Decode *data* into a fresh *msg_class* with *codec*."""
        return codecs.do_decode(data, msg_class, codec, **kwargs)
