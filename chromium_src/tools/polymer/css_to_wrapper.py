# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils
import argparse
import os

from os import path
from brave_chromium_utils import get_webui_overridden_but_referenced_files


@override_utils.override_method(argparse.ArgumentParser)
def parse_args(self, original_method, argv):
    args = original_method(self, argv)
    in_folder = path.normpath(path.join(os.getcwd(), args.in_folder))

    for file in get_webui_overridden_but_referenced_files(
            in_folder, args.in_files):
        args.in_files.append(file)
    return args
