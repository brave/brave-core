# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os

import brave_node
import override_utils


@override_utils.override_function(globals())
def Run(_original_function, **kwargs):
    node_args = [
        brave_node.PathInNodeModules('eslint', 'bin', 'eslint'),
        '--quiet',
        '--resolve-plugins-relative-to',
        brave_node.PathInNodeModules(),
    ]
    if os.environ.get('PRESUBMIT_FIX') == '1':
        node_args += ['--fix']
    # Don't ignore files/directories starting with a dot.
    node_args += ['--ignore-pattern', '!.*']
    node_args += kwargs['args']

    return brave_node.RunNode(node_args)
