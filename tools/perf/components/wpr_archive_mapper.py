# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

from components.path_util import GetPageSetsDataPath

_WPR_ARCHIVES_TO_REPLACE = [
    'system_health_desktop.json',
    'system_health_mobile.json',
    'rendering_desktop.json',
    'rendering_mobile.json',
    'jetstream2.json',
]


# An single endpoint to replace Chromium wpr .json archives.
# The archives outside the list are used as is.
# The function is used in chromium story_set.py via chromium_src.
def get_archive_data_file(original_archive_path: str) -> str:
  filename = os.path.basename(original_archive_path)
  for archive_filename in _WPR_ARCHIVES_TO_REPLACE:
    if filename == archive_filename:
      return GetPageSetsDataPath(filename)
  return original_archive_path
