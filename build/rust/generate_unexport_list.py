#!/usr/bin/env python3
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import os
import sys

# Set up path to load build_utils.py which enables us to do
# atomic output that's maximally compatible with ninja.
sys.path.append(
  os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir,
               os.pardir, os.pardir, 'build', 'android', 'gyp'))
from util import build_utils

import argparse
import subprocess


def run(exe, input, output):
  cmdargs = [exe]
  cmdargs += ["--extern-only"]
  cmdargs += ["--no-sort"]
  cmdargs += ["--just-symbol-name"]
  cmdargs += [input]
  job = subprocess.run(cmdargs, capture_output=True)
  if job.returncode != 0:
    messages = job.stderr.decode('utf-8')
    if messages.rstrip():
      print(messages, file=sys.stderr)
    return job.returncode
  with build_utils.AtomicOutput(output) as output:
    output.write(job.stdout)
  return 0


def main():
  parser = argparse.ArgumentParser("generate_unexport_list.py")
  parser.add_argument("--bin_path", required=True),
  parser.add_argument("--input", required=True)
  parser.add_argument("--output", required=True)
  args = parser.parse_args()

  exe = os.path.join(args.bin_path, "nm")
  return run(exe, args.input, args.output)


if __name__ == '__main__':
  sys.exit(main())
