# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import multiprocessing
import sys

import override_utils


@override_utils.override_function(multiprocessing)
def cpu_count(original_function):
    result = original_function()

    # Ensure python on MacOS does not hang because of big multiprocessing pool.
    # https://github.com/brave/brave-browser/issues/22692
    if sys.platform == 'darwin':
        return min(result, 4)

    return result
