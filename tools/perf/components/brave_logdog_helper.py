# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.


# pylint: disable=no-self-use
class FakeStream:

  def close(self):
    return

  def write(self, _chunk):
    return

  def get_viewer_url(self):
    return None


def text(name, data, content_type=None):  # pylint:disable=unused-argument
  return name


def open_text(_name):
  return FakeStream()
