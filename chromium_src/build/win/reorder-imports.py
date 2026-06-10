# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import glob

import override_utils


@override_utils.override_function(glob, name='iglob')
def iglob(original_function, *args, **kwargs):
    for fname in original_function(*args, **kwargs):
        # The .rsp (response file) can still be present while this action runs,
        # make sure to skip it as it's not a valid input file.
        if fname.endswith('.rsp'):
            continue
        yield fname
