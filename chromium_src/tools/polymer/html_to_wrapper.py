# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils

POLYMER_OVERRIDING_TOKEN = '//resources/brave/polymer_overriding.js'
LIT_OVERRIDING_TOKEN = '//resources/brave/lit_overriding.js'


@override_utils.override_function(globals())
def detect_template_type(original_method, definition_file):
    with open(definition_file, encoding='utf-8', mode='r') as f:
        content = f.read()

        if POLYMER_OVERRIDING_TOKEN in content:
            return 'polymer'
        if LIT_OVERRIDING_TOKEN in content:
            return 'lit'

    return original_method(definition_file)
