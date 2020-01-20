# Copyright (c) 2019 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

import argparse
import os
import sys
import subprocess

def main():
  print 'archiving object files...'
  argument_parser = argparse.ArgumentParser()
  argument_parser.add_argument('root_out_dir')
  argument_parser.add_argument('target_obj_files_name')
  args = argument_parser.parse_args()

  if os.path.exists(args.target_obj_files_name + '.tar.7z'):
      os.remove(args.target_obj_files_name + '.tar.7z')
  subprocess.check_output([
       'sh',
       '../../brave/build/android/makeArchive7z.sh',
       args.root_out_dir,
       args.target_obj_files_name
   ])

if __name__ == '__main__':
  sys.exit(main())
