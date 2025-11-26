# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
import os
import sys
import subprocess

TESTING_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                           os.pardir))


def main() -> int:
  python3_dir_path = os.path.dirname(sys.executable)
  os.environ["PATH"] = python3_dir_path + os.pathsep + os.environ["PATH"]
  os.environ['PYTHONPATH'] = ''

  # set PYTHONPATH explicitly to avoid issues with src/third_party/depot_tools
  brave_dir = os.path.join(TESTING_DIR, os.pardir, os.pardir, os.pardir)
  os.environ['PYTHONPATH'] = os.path.join(brave_dir, 'script')

  args = [sys.executable, '-m', 'pytype', '.', '--keep-going', '--jobs=auto']
  try:
    subprocess.check_call(args, cwd=TESTING_DIR)
  except subprocess.CalledProcessError:
    return 1
  return 0


if __name__ == '__main__':
  sys.exit(main())
