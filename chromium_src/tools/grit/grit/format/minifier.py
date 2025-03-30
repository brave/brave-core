# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import override_utils


@override_utils.override_function(globals())
def Minify(original_function, source, filename):
    if 'gen/brave/web-ui-opaque_ke/' in filename:
        return source
    return original_function(source, filename)
