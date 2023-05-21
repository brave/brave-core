#!/usr/bin/env python3
#
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import subprocess
import sys

# Set up path to be able to import action_helpers
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir,
                 os.pardir, 'build'))
import action_helpers  # pylint: disable=wrong-import-position


def run(exe, input_file, output_file):
    cmdargs = [exe]
    cmdargs += ["--extern-only"]
    cmdargs += ["--no-sort"]
    cmdargs += ["--just-symbol-name"]
    cmdargs += [input_file]
    job = subprocess.run(cmdargs, capture_output=True, check=False)
    if job.returncode != 0:
        messages = job.stderr.decode('utf-8')
        if messages.rstrip():
            print(messages, file=sys.stderr)
        return job.returncode
    with action_helpers.atomic_output(output_file) as output:
        output.write(job.stdout)
    return 0


def main():
    parser = argparse.ArgumentParser("generate_unexport_list.py")
    parser.add_argument("--bin_path", required=True)
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    exe = os.path.join(args.bin_path, "llvm-nm")
    return run(exe, args.input, args.output)


if __name__ == '__main__':
    sys.exit(main())
