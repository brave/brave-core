# Copyright 2022 The Brave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import override_utils

@override_utils.override_method(PolicyJson)
def SetDefines(self, original_method, defines):
  if '_google_chrome' in defines:
    self._config = {
      'build': 'brave',
      'app_name': 'Brave',
      'frame_name': 'Brave Frame',
      'os_name': 'Google Chrome OS'
    }
  else:
    original_method(self, defines)
