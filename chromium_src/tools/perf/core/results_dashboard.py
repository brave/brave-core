# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A inline part of result_dashboard.py"""
import re
import os
import sys
import subprocess

import override_utils

from core import path_util  # pylint: disable=import-error


def BraveAuthTokenGeneratorCallback():
    """A substitution for LuciAuthTokenGeneratorCallback()"""
    vpython_name = 'vpython3.bat' if sys.platform == 'win32' else 'vpython3'
    vpython_path = os.path.join(path_util.GetChromiumSrcDir(), 'brave',
                                'vendor', 'depot_tools', vpython_name)
    args = [
        vpython_path,
        os.path.join(path_util.GetChromiumSrcDir(), 'brave', 'tools', 'perf',
                     'dashboard_auth.py')
    ]

    p = subprocess.Popen(args,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         universal_newlines=True)
    if p.wait() == 0:
        return p.stdout.read().strip()
    raise RuntimeError(
        'Error generating authentication token.\nStdout: %s\nStder:%s' %
        (p.stdout.read(), p.stderr.read()))


@override_utils.override_function(globals())
def SendResults(original_function, *args, **kwargs):
    kwargs['token_generator_callback'] = BraveAuthTokenGeneratorCallback
    return original_function(*args, **kwargs)


@override_utils.override_function(globals())
def _MakeBuildStatusUrl(original_function, project, buildbucket, buildername,
                        buildnumber):
    m = re.match('brave/refs/tags/(.+)', buildername)
    if m:
        URL = 'https://github.com/brave/brave-browser/releases/tag/%s'
        return URL % m.group(1)
    return original_function(project, buildbucket, buildername, buildnumber)
