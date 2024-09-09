# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import shutil
import argparse
import atexit
import os

import override_utils


def remove_copied_source(sources):
    for source in sources:
        if os.path.exists(source):
            os.remove(source)


@override_utils.override_method(argparse.ArgumentParser)
def parse_args(_self, original_method, *args, **kwargs):
    opts = original_method(_self, *args, **kwargs)

    sources = []
    copied_sources = []
    for source in opts.sources or []:
        if source.find("/brave") != -1:
            dest_file = source.replace("/brave", "")
            shutil.copy2(source, dest_file)
            sources.append(dest_file)
            copied_sources.append(dest_file)
        else:
            sources.append(source)

    opts.sources = sources

    if copied_sources:
        atexit.register(remove_copied_source, copied_sources)

    return opts
