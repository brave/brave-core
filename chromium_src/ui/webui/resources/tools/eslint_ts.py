# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import shlex

import override_utils


@override_utils.override_function(globals())
def main(original_function, argv):
    rsp_parser = argparse.ArgumentParser()
    rsp_parser.add_argument('--rsp', required=False)
    rsp_args, _ = rsp_parser.parse_known_args(argv)

    if rsp_args.rsp:
        with open(rsp_args.rsp, 'r') as f:
            # Do not prepend argv[0], because original script strips it.
            argv = shlex.split(f.read())

    return original_function(argv)
