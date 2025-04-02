# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import os
import re

import brave_chromium_utils
import override_utils


def ensure_hardlink(src, dst):
    src = os.path.abspath(src) if not os.path.isabs(src) else src
    dst = os.path.abspath(dst) if not os.path.isabs(dst) else dst

    try:
        os.link(src, dst)
    except FileExistsError:
        if not os.path.samefile(src, dst):
            # recreating link if dst is not pointing to the src
            os.unlink(dst)
            os.link(src, dst)


# Here we put our files in the devtools source directory due to the limitations
# of 'rootDir' in the configuration files.
@override_utils.override_method(argparse.ArgumentParser)
def parse_args(_self, original_method, *args, **kwargs):
    opts = original_method(_self, *args, **kwargs)

    sources = []
    for source in opts.sources or []:
        override = brave_chromium_utils.get_chromium_src_override(source)
        if os.path.exists(override):
            dest_file = re.sub(r'\.ts$', '.patch.ts', source)
            ensure_hardlink(override, dest_file)
            sources.append(source)
            sources.append(dest_file)

        elif source.find("/brave/") != -1:
            dest_file = source.replace("/brave/", "/", 1)
            ensure_hardlink(source, dest_file)
            sources.append(dest_file)
        else:
            sources.append(source)

    opts.sources = sources

    return opts
