# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from functools import cache
from subprocess import check_output

import override_utils
import platform


@override_utils.override_function(globals())
def macos_version(original_function):
    if platform.system() == 'Darwin':
        # The typical case.
        return original_function()
    # We are cross-compiling.
    return [int(x) for x in _get_macos_release().split('.')]


@cache
def _get_macos_release():
    # Fetch the release version from the remote macOS host. Here, `sw_vers` is a
    # fake binary on the PATH that achieves this.
    return check_output(['sw_vers', '-productVersion'], text=True).rstrip()
