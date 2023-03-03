# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
import os
import re
from typing import Tuple, Optional

from components.browser_type import BrowserType, BraveVersion


def ParseTarget(target: str) -> Tuple[Optional[BraveVersion], str]:
  m = re.match(r'^(v\d+\.\d+\.\d+)(?::(.+)|$)', target)
  if not m:
    return None, target
  tag = BraveVersion(m.group(1))
  location = m.group(2)
  logging.debug('Parsed tag: %s, location : %s', tag, location)
  return tag, location


def PrepareBinary(out_dir: str, tag: Optional[BraveVersion],
                  location: Optional[str], browser_type: BrowserType) -> str:
  if location:  # local binary
    if os.path.exists(location):
      return location
    raise RuntimeError(f'{location} doesn\'t exist')
  assert (tag is not None)
  return browser_type.DownloadBrowserBinary(tag, out_dir)
