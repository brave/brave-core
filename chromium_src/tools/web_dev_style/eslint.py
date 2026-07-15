# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

import brave_chromium_utils
import override_utils

with brave_chromium_utils.sys_path('//third_party/node'):
    import node


@override_utils.override_function(globals())
def Run(_original_function, **kwargs):
    node_args = [
        brave_chromium_utils.wspath('//brave/node_modules/eslint/bin/eslint'),
        '--quiet'
    ]
    if os.environ.get('PRESUBMIT_FIX') == '1':
        node_args += ['--fix']
    # Don't ignore files/directories starting with a dot.
    node_args += ['--ignore-pattern', '!.*']
    node_args += kwargs['args']

    return node.RunNodeRaw(node_args)
