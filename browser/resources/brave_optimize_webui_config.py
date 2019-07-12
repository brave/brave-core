# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import os

def vulcanize_base_args():
  return [
    # Exclude anything that contains dynamic content, e.g. l18n strings
    # or is large and can be cached across webui pages.
    '--exclude', 'chrome://brave-resources/fonts/muli.css',
    '--exclude', 'chrome://brave-resources/fonts/poppins.css',
  ]

def url_mappings(src_path):
  # Provide mappings from URL paths to FS locations
  br_resources_path = os.path.join(src_path, 'brave', 'ui', 'webui', 'resources')
  return [
    ('chrome://brave-resources/', br_resources_path),
  ]
