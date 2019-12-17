# Copyright 2019 The Brave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import sys
import subprocess
import tempfile

def main():
  argument_parser = argparse.ArgumentParser()
  argument_parser.add_argument('zipalign_path')
  argument_parser.add_argument('apksigner_path')
  argument_parser.add_argument('unsigned_apk_path')
  argument_parser.add_argument('key_path')
  argument_parser.add_argument('key_passwd')
  argument_parser.add_argument('prvt_key_passwd')
  argument_parser.add_argument('key_name')
  args = argument_parser.parse_args()

  with tempfile.NamedTemporaryFile() as staging_file:
    subprocess.check_output([
        args.zipalign_path, '-p', '-f', '4',
        args.unsigned_apk_path, staging_file.name])
    subprocess.check_output([
        args.apksigner_path, 'sign',
        '--in', staging_file.name,
        '--out', args.unsigned_apk_path,
        '--ks', args.key_path,
        '--ks-key-alias', args.key_name,
        '--ks-pass', 'pass:' + args.key_passwd,
        '--key-pass', 'pass:' + args.prvt_key_passwd,
    ])

if __name__ == '__main__':
  sys.exit(main())
