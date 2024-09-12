# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import re

import override_utils


# If source file has a brave/chromium_src patch we should add
# the result of tsc compilation in the grd file.
@override_utils.override_function(globals())
def parse_args(original_func, *args, **kwargs):
    parsed_args = original_func(*args, **kwargs)

    source_files = []
    for filename in parsed_args.source_files:
        source_files.append(filename)

        patch_name = re.sub(r'\.js$', '.patch.js', filename)
        patch = os.path.join(os.path.dirname(parsed_args.output_filename),
                             patch_name)
        if filename != patch_name and os.path.exists(patch):
            source_files.append(patch_name)

    parsed_args.source_files = source_files

    return parsed_args
