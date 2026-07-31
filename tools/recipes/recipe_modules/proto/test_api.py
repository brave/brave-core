# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `proto`: seed what a proto output placeholder hands back.

Seed the message itself -- the placeholder knows its own codec, so a test never
has to spell the encoding out:

    api.step_data('read config', api.proto.output(Config(name='release')))
"""

from __future__ import annotations

from typing import Any

from google.protobuf.message import Message

from recipe_test_api import RecipeTestApi, placeholder_step_data

from . import codec as codecs


class ProtoTestApi(RecipeTestApi):
    """Seed the simulated data of a `proto` output placeholder."""

    @staticmethod
    def encode(proto_msg: Message, codec: codecs.Codec | str, **kwargs) -> Any:
        """Same as `api.proto.encode`."""
        return codecs.do_encode(proto_msg, codec, **kwargs)

    @staticmethod
    def decode(data: Any, msg_class: type[Message], codec: codecs.Codec | str,
               **kwargs) -> Message:
        """Same as `api.proto.decode`."""
        return codecs.do_decode(data, msg_class, codec, **kwargs)

    @placeholder_step_data
    def output(self,
               proto_msg: Message,
               retcode: int | None = None,
               name: str | None = None):
        """Seed an `api.proto.output()` placeholder to return *proto_msg*."""
        if not isinstance(proto_msg, Message):
            raise ValueError('expected a protobuf message, got '
                             f'{type(proto_msg)}')
        return proto_msg, retcode, name

    @placeholder_step_data('output')
    def invalid(self,
                raw_data: str = 'i are not protoh',
                retcode: int | None = None,
                name: str | None = None):
        """Seed an `api.proto.output()` placeholder with unparseable data.

        For exercising what a recipe does when a step writes garbage: the
        placeholder's result is then `None`.
        """
        return raw_data, retcode, name

    @placeholder_step_data('output')
    def backing_file_missing(self,
                             retcode: int | None = None,
                             name: str | None = None):
        """Seed an output placeholder as if the step never wrote its file.

        Only meaningful for a placeholder created with `leak_to`; without it the
        engine creates the backing file itself, so it is always there.
        """
        return None, retcode, name
