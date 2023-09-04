# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import json
from pathlib import Path
import os


def get_root():
    return f'{Path(__file__).parent}/../..'


def get_root_relative(icon_path):
    return "//" + os.path.relpath(os.path.abspath(icon_path), get_root()) \
      .replace(os.sep, '/')


leo_overrides = {}


def maybe_load_overrides():
    """Loads the Leo Overrides into the global `leo_overrides` variable"""
    if len(leo_overrides) != 0:
        return

    brave_root = os.path.join(get_root(), 'brave')
    overrides_file_path = f'{brave_root}/vector_icons/leo_overrides.json'

    data = json.load(open(overrides_file_path))

    for icon, leo_override in data.items():
        if not icon.startswith("//"):
            raise Exception("Only absolute paths are supported")
        leo_overrides[icon] = \
          f'../../brave/node_modules/@brave/leo/icons-skia/{leo_override}.icon'


def get_icon_path(icon_path, alt_working_directory):
    """Determines where to read the icon from. Options are:
    1. The Leo `icons-skia` folder, mapping `icon_path` to a Leo icon, as per
       the `leo_overrides.json` file.
    2. The Brave equivalent of the the Chromium icon path.
    3. The Chromium Path (this is the default, if no override is specified).

    Args:
        icon_path: The path to the chromium icon
        alt_working_directory: The Brave overrides folder for this icon.
  """
    maybe_load_overrides()

    # Check for alternative path
    alt_icon_path = os.path.join(alt_working_directory,
                                 os.path.basename(icon_path))

    chromium_path = get_root_relative(icon_path)
    if chromium_path in leo_overrides:
        return leo_overrides[chromium_path]
    if os.path.exists(alt_icon_path):
        return alt_icon_path

    return icon_path
