# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import glob

import override_utils


@override_utils.override_function(globals())
def main(original_function, argv):

    @override_utils.override_method(argparse.ArgumentParser)
    # pylint: disable=unused-variable
    def parse_args(self, original_method, argv):
        args = original_method(self, argv)

        in_folder = os.path.normpath(os.path.join(os.getcwd(), args.in_folder))
        for file in glob.glob(os.path.join(in_folder, '**/*chromium*.js')):
            args.in_files.append(os.path.relpath(file, in_folder))
        return args

    original_function(argv)
