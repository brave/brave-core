# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils
import glob
import argparse
import os

from os import path


@override_utils.override_function(globals())
def main(original_function, argv):

    @override_utils.override_method(argparse.ArgumentParser)
    # pylint: disable=unused-variable
    def parse_args(self, original_method, argv):
        args = original_method(self, argv)
        in_folder = path.normpath(path.join(os.getcwd(), args.in_folder))

        args.in_files.extend([
            path.relpath(f, args.in_folder)
            for f in glob.glob(path.join(args.in_folder, '**/*-chromium.css'))
        ])
        return args

    original_function(argv)
