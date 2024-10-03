# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import shutil
import argparse
import atexit
import os
import re

import brave_chromium_utils
import override_utils


def remove_copied_sources(sources):
    for source in sources:
        if os.path.exists(source):
            os.remove(source)


# Here we put our files in the devtools source directory due to the limitations
# of 'rootDir' in the configuration files. After the build, the copied files
# are deleted from the chrome directories.
@override_utils.override_method(argparse.ArgumentParser)
def parse_args(_self, original_method, *args, **kwargs):
    opts = original_method(_self, *args, **kwargs)

    sources = []
    copied_sources = []
    for source in opts.sources or []:
        override = brave_chromium_utils.get_chromium_src_override(source)
        if os.path.exists(override):
            dest_file = re.sub(r'\.ts$', '.patch.ts', source)
            shutil.copy2(override, dest_file)
            sources.append(source)
            sources.append(dest_file)
            copied_sources.append(dest_file)

        elif source.find("/brave/") != -1:
            dest_file = source.replace("/brave/", "/", 1)
            shutil.copy2(source, dest_file)
            sources.append(dest_file)
            copied_sources.append(dest_file)
        else:
            sources.append(source)

    opts.sources = sources

    if copied_sources:
        atexit.register(remove_copied_sources, copied_sources)

    return opts
