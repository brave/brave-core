# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import brave_chromium_utils
import override_utils
import collections


@override_utils.override_function(globals())
def read_config(original_function):
    jinja_dir, config_file, config = original_function()
    config.protocol.options.append(
        collections.namedtuple("X", ["domain"])("Brave"))

    return (jinja_dir, config_file, config)
