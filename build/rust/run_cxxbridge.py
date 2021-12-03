#!/usr/bin/env vpython3

# Copyright 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

from rust_deps_config import RUST_DEPS_PACKAGE_VERSION

# Set up path to load build_utils.py which enables us to do
# atomic output that's maximally compatible with ninja.
sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir,
                 os.pardir, os.pardir, 'build', 'android', 'gyp'))
from util import build_utils

import argparse
import subprocess


def run(cargo_path, args, output, is_header):
  cargo_home = os.path.join(cargo_path, RUST_DEPS_PACKAGE_VERSION)
  exe = os.path.abspath(os.path.join(cargo_home, 'bin', 'cxxbridge'))

  if sys.platform == "win32":
    exe = exe + '.exe'

  cmdargs = [exe]
  cmdargs.extend(args)
  if is_header:
    cmdargs.extend(["--header"])
  job = subprocess.run(cmdargs, capture_output=True)
  messages = job.stderr.decode('utf-8')
  if messages.rstrip():
    print(messages, file=sys.stderr)
  if job.returncode != 0:
    return job.returncode
  with build_utils.AtomicOutput(output) as output:
    output.write(job.stdout)
  return 0


def main():
  parser = argparse.ArgumentParser("run_cxxbridge.py")
  parser.add_argument('--cargo_path', required=True)
  parser.add_argument("--cc", help="output cc file", required=True)
  parser.add_argument("--header", help="output h file", required=True)
  parser.add_argument('args',
                      metavar='args',
                      nargs='+',
                      help="Args to pass through")
  args = parser.parse_args()
  v = run(args.cargo_path, args.args, args.cc, False)
  if v != 0:
    return v
  v = run(args.cargo_path, args.args, args.cc, False)


if __name__ == '__main__':
  sys.exit(main())
