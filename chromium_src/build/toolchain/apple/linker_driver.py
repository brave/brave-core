# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import subprocess

import override_utils


def _is_lld_with_lto_call(*popenargs, **kwargs):
    cmd = kwargs.get("args")
    if cmd is None:
        cmd = popenargs[0]
    return cmd[0].endswith(
        '/clang++') and '-fuse-ld=lld' in cmd and '-flto=thin' in cmd


@override_utils.override_function(subprocess)
def check_call(original_function, *popenargs, **kwargs):
    try:
        return original_function(*popenargs, **kwargs)
    except subprocess.CalledProcessError as e:
        # Restart lld link if a previous run has failed.
        # https://github.com/brave/brave-browser/issues/23734
        if _is_lld_with_lto_call(*popenargs, **kwargs):
            print('Restarting lld because of the failure:', e)
            return original_function(*popenargs, **kwargs)
        raise
