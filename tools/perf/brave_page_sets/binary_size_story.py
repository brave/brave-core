# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at https://mozilla.org/MPL/2.0/.

# Used as part of chromium src/tools/perf/page_sets/
# Therefore has the same code conventions (including pylint).
# pylint: disable=line-too-long, import-error, super-with-arguments
# pylint: disable=no-name-in-module, too-few-public-methods

from page_sets import press_story
from telemetry import story

class BinarySizeStory(press_story.PressStory):
  URL = 'about:blank'
  NAME = 'BinarySize'
  binary_size = 0

  def __init__(self, page_set, binary_size):
    super(BinarySizeStory, self).__init__(page_set)
    self.binary_size = binary_size

  def Run(self, shared_state):
    self.AddMeasurement(
      'binary_size', 'sizeInBytes_smallerIsBetter',
      self.binary_size,
      description='The binary size in bytes (apk, dmg, exe, zip)')


class BinarySizeStorySet(story.StorySet):
  def __init__(self, binary_size):
    super(BinarySizeStorySet,
          self).__init__()
    self.AddStory(BinarySizeStory(self, binary_size))
