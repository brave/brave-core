# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
import os
import re
from typing import Tuple, Optional

from components.browser_type import BrowserType


def ParseTarget(target: str) -> Tuple[Optional[str], str]:
  m = re.match(r'^(v\d+\.\d+\.\d+)(?::(.+)|$)', target)
  if not m:
    return None, target
  logging.debug('Parsed tag: %s, location : %s', m.group(1), m.group(2))
  return m.group(1), m.group(2)


def PrepareBinary(out_dir: str, tag: str, location: Optional[str],
                  browser_type: BrowserType) -> str:
  if location:  # local binary
    if os.path.exists(location):
      return location
    raise RuntimeError(f'{location} doesn\'t exist')
  return browser_type.DownloadBrowserBinary(tag, out_dir)
