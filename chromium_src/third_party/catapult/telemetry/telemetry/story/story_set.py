# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os
from typing import Optional

import brave_chromium_utils
import override_utils

from py_utils import cloud_storage

with brave_chromium_utils.sys_path('//brave/tools/perf'):
    from components.path_util import GetPageSetsDataPath, GetBraveDir

_WPR_ARCHIVES_TO_REPLACE = [
    'system_health_desktop.json',
    'system_health_mobile.json',
    'rendering_desktop.json',
    'rendering_mobile.json',
    'jetstream2.json',
]


# An single endpoint to replace Chromium wpr .json archives.
# The function is used in chromium story_set.py via chromium_src.
def _get_archive_data_file(original_archive_path: str) -> Optional[str]:
    filename = os.path.basename(original_archive_path)
    for archive_filename in _WPR_ARCHIVES_TO_REPLACE:
        if filename == archive_filename:
            return GetPageSetsDataPath(filename)
    return None


@override_utils.override_method(StorySet)
def __init__(self,
             original_method,
             archive_data_file='',
             cloud_storage_bucket=None,
             base_dir=None,
             *args,
             **kwargs):
    original_method(self, archive_data_file, cloud_storage_bucket, base_dir,
                    args, kwargs)
    if archive_data_file == '':
        return
    archive_path = os.path.join(self._base_dir, archive_data_file)
    new_archive_path = _get_archive_data_file(archive_path)
    if new_archive_path:
        self._archive_data_file = os.path.relpath(new_archive_path,
                                                  self._base_dir)
    else:
        if cloud_storage_bucket != cloud_storage.PUBLIC_BUCKET and not archive_path.startswith(
                GetBraveDir()):
            raise RuntimeError(
                f'No brave replacement for private WPR {archive_path}')
