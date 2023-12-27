# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A inline part of generate_gradle.py"""
import os

import override_utils

_GENERATED_JAVA_SUBDIR = 'generated_java'
_INPUT_SRC_JARS = 'input_srcjars'
_BRAVE_GEN_COMPONENTS = os.path.join('gen', 'brave', 'components')


@override_utils.override_method(_ProjectEntry)
def GeneratedJavaSubdir(self, _):
    gen_path = os.path.join('gen', self.GradleSubdir(), _GENERATED_JAVA_SUBDIR)
    if gen_path.startswith(_BRAVE_GEN_COMPONENTS):
        gen_path_src_jars = os.path.join(gen_path, _INPUT_SRC_JARS)
        if os.path.exists(_RebasePath(gen_path_src_jars)):
            gen_path = gen_path_src_jars
    return _RebasePath(gen_path)
