# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

import brave_chromium_utils
import override_utils


with brave_chromium_utils.sys_path('//brave/tools/perf'):
    from components.wpr_archive_mapper import get_archive_data_file

@override_utils.override_method(StorySet)
def __init__(self,
               original_method,
               archive_data_file='',
               cloud_storage_bucket=None,
               base_dir=None,
               *args,
               **kwargs):
  original_method(self, archive_data_file, cloud_storage_bucket, base_dir, args, kwargs)
  archive_path = os.path.join(self._base_dir, archive_data_file)
  new_archive_path = get_archive_data_file(archive_path)
  self._archive_data_file = os.path.relpath(new_archive_path, self._base_dir)
